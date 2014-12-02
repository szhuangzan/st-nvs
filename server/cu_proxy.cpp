#include "cu_proxy.hpp"
#include "server.hpp"

#include <cassert>


uint32 cu_proxy_t::_cu_dev_count = 0;
cu_proxy_t::cu_proxy_t()
:_pu_proxy(0)
,_video_msg_wrap(0)
{
	_cu_dev_count++;
}

cu_proxy_t::~cu_proxy_t()
{
	_cu_dev_count--;
}
void cu_proxy_t::init(pu_proxy_t* pu_proxy)
{
	_pu_proxy = pu_proxy;
	_pu_proxy->add();
	
}

void cu_proxy_t::send_msg(net_port_msg_t*arg)
{
	if(_socket_fd.is_vaild())
	{
		_msg_queue.push(arg);
		notify();
	}
}

void* cu_proxy_t::handle_send(void*arg)
{
	cu_proxy_t* proxy = (cu_proxy_t*)arg;
	bool  exit = false;
	while(1)
	{
		proxy->wait();
		if(!proxy->_socket_fd.is_vaild())
		{
			printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
			exit = true;
			break;
		}

		{
			std::vector<net_port_msg_t*> msg_vec;
			proxy->_msg_queue.backup(msg_vec);

			std::vector<net_port_msg_t*>::iterator iter = msg_vec.begin();
			for(; iter != msg_vec.end();)
			{
				net_port_msg_t* msg = (*iter);
				if(!exit)
				{
					net_port_header_t hdr =  msg->hdr;
					uint32 bodylen =hdr.bodylen;
					uint32 sendlen = bodylen+sizeof(net_port_header_t);
					if(hdr.cmdno==hmcmd_recv_video)
					{
						hdr.serialid = proxy->_video_serial;
					}

					char* ptr= (char*)malloc(sendlen);
					
					assert(sendlen);
					assert(ptr);
					memset(ptr, 0, sendlen);

					transfer_out(hdr);

					memcpy(ptr, (char*)&hdr,sizeof(net_port_header_t));
					if(bodylen >0) memcpy(ptr+sizeof(net_port_header_t), msg->body, bodylen);

					if (!proxy->_socket_fd.is_vaild() || st_write(proxy->_socket_fd.get_fd(), ptr , sendlen, SEC2USEC(10)) != sendlen)
					{
						printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
						exit = true;
					}
					free(ptr);
				}
				msg->release();
				iter = msg_vec.erase(iter);
			}
			if(exit) break;
		}
	
	}
	if(exit)
	{
		proxy->_msg_queue.clear();
		assert(proxy->_msg_queue.size() == 0);
		close(proxy);
		
	}
	return NULL;
}

void* cu_proxy_t::handle_recv(void*arg)
{
	cu_proxy_t* proxy = (cu_proxy_t*)arg;
	while (1)
	{
		net_port_header_t hdr;
		if (!proxy->_socket_fd.is_vaild() || st_read(proxy->_socket_fd.get_fd(), &hdr, sizeof(net_port_header_t), SEC2USEC(REQUEST_TIMEOUT)) < 0) 
		{
			printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
			break;
		}

		if(!check_net(proxy))
		{	
			printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
			break;
		}

		net_port_msg_t* msg = new net_port_msg_t;
		transfer_in(hdr);
		msg->hdr = hdr;

		if(hdr.bodylen>0)
		{
			msg->body = (char*)malloc(hdr.bodylen);
			if (!proxy->_socket_fd.is_vaild() || st_read_fully(proxy->_socket_fd.get_fd(), msg->body, hdr.bodylen, SEC2USEC(REQUEST_TIMEOUT)) < 0) 
			{
				msg->release();
				printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
				break;
			}
			if(msg->body)
			{
				free(msg->body);
			}
		}

		if(!check_net(proxy))
		{	
			msg->release();
			printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
			break;
		}

		if(hdr.cmdno == hmcmd_heartbeat)
		{
			msg->hdr.cmdno = 0x80000000|hmcmd_heartbeat;
			msg->hdr.bodylen=0;
			proxy->send_msg(msg);
		}
		else if(hdr.cmdno == hmcmd_open_video)
		{
			proxy->_video_msg_wrap = new cu_msg_wrap_t;
			msg->cond = st_cond_new();
			proxy->_video_msg_wrap->msg = msg;
			proxy->_video_msg_wrap->cu_proxy = proxy;
			st_thread_create(open_real_video, proxy->_video_msg_wrap, 0,0);
		}
		else
		{
			printf("assert...\n");
			msg->release();
			break;;
		}
	}

	close(proxy);
	return NULL;
}


void cu_proxy_t::close(cu_proxy_t* proxy)
{
	if(proxy->_socket_fd.is_vaild())
	{
		proxy->_socket_fd.uninit();
		if(proxy->_video_msg_wrap)
		{
			st_cond_broadcast(proxy->_video_msg_wrap->msg->cond);
			proxy->_video_msg_wrap = 0;
		}
		proxy->notify();
	}
	proxy->release();
}

void*cu_proxy_t::open_real_video(void*arg)
{
	static bool flag = false;
	cu_msg_wrap_t* msg_wrap = (cu_msg_wrap_t*)arg;
	msg_wrap->cu_proxy->add();
	net_port_msg_t* msg = msg_wrap->msg;
	uint32 serial = msg->hdr.serialid;
	msg_wrap->cu_proxy->_video_serial = serial;
	net_port_msg_t* tmp =  msg_wrap->cu_proxy->_pu_proxy->open_real_video(msg_wrap->cu_proxy, msg);
	if(tmp)
	{
		msg->hdr = tmp->hdr;
		if(tmp->hdr.bodylen>0)
		{
			msg->body = (char*)malloc(tmp->hdr.bodylen);
			memcpy(msg->body, tmp->body, tmp->hdr.bodylen);
		}
		msg->hdr.serialid = serial;
		msg_wrap->cu_proxy->send_msg(msg);
		msg->add();
	}
	else
	{
		assert(flag);
		flag = true;
		msg->hdr.serialid = serial;
		msg->add();
		msg_wrap->cu_proxy->send_msg(msg);
	}
	st_cond_wait(msg->cond);
	if(msg_wrap->cu_proxy->_pu_proxy)
	{
		msg_wrap->cu_proxy->_pu_proxy->close_real_video(msg_wrap->cu_proxy);
	}
	msg->release();
	msg_wrap->cu_proxy->uninit();
	msg_wrap->cu_proxy->release();
	
	printf("%s:%d -> %x\n",__FUNCTION__,__LINE__, st_thread_self());
	return NULL;
}

void*cu_proxy_t:: send_proxy(void*arg)
{
	cu_msg_wrap_t* msg_wrap = (cu_msg_wrap_t*)arg;
	net_port_msg_t* msg = msg_wrap->msg;
	msg_wrap->cu_proxy->send_msg(msg);
	st_cond_destroy(msg->cond);
	return NULL;
}
void cu_proxy_t::run(st_netfd_t cli_fd)
{
	_socket_fd.init(cli_fd);
	add();
	add();
	st_thread_create(handle_recv, this, 0,0);
	st_thread_create(handle_send, this, 0,0);
}