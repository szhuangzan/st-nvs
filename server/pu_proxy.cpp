#include "pu_proxy.hpp"
#include "server.hpp"
#include "cu_proxy.hpp"
#include <cassert>
#include <algorithm>
#include "server.hpp"

uint32 pu_proxy_t::_pu_dev_count = 0;
pu_proxy_t::pu_proxy_t(std::string sn)
:_sn(sn)
,_real_video_cmd(0)
{
	_pu_dev_count++;
	_wait_queue.clear();
}

pu_proxy_t::~pu_proxy_t()
{
	_pu_dev_count--;
	_real_video_cmd->release();
	ServerHandler::unregister(_sn);
}

void pu_proxy_t::init()
{
	

}
net_port_msg_t* pu_proxy_t::open_real_video(cu_proxy_t*cu, net_port_msg_t* msg)
{
	{
		cu->add();
		_pu_video_proxy_vec.push_back(cu);
	}
	
	if(_real_video_cmd)
	{
		_real_video_cmd->add();
		return _real_video_cmd;
	}
	else
	{
		send_msg(msg);
	}
	return NULL;
}

void pu_proxy_t::close_real_video(cu_proxy_t*cu)
{
	std::vector<cu_proxy_t*>::const_iterator iter = std::find(_pu_video_proxy_vec.begin(),_pu_video_proxy_vec.end(),cu);
	if(iter != _pu_video_proxy_vec.end())
	{
		(*iter)->release();
		_pu_video_proxy_vec.erase(iter);
		_real_video_cmd->release();
	}
}


void pu_proxy_t::send_msg(net_port_msg_t* msg)
{
	if(_socket_fd.is_vaild())
	{
		_msg_queue.push(msg);
		notify();
		if(msg->cond) 
		{
			st_cond_timedwait(msg->cond,SEC2USEC(10));
			if(st_errno() == ETIMEDOUT)
			{
				printf("Time Out   = %x\n" , msg->hdr.cmdno);
			}
		}
	}
}

void* pu_proxy_t::handle_send(void*arg)
{
	pu_proxy_t* proxy = (pu_proxy_t*)arg;
	uint32 serial = 1;
	bool exit = false;
	st_thread_create(heart_thread, (void*)proxy, 0,0);
	while(1)
	{
		proxy->wait();
		if(!proxy->_socket_fd.is_vaild())
		{
			printf("%s:%d -> %d\n",__FUNCTION__,__LINE__, st_thread_self());
			break;
		}

		{
			std::vector<net_port_msg_t*> msg_vec;
			proxy->_msg_queue.backup(msg_vec);

			for(uint i=0;i<msg_vec.size();i++)
			{
				if(!exit)
				{
					char send_buf[1024] = {};
					net_port_header_t hdr = msg_vec[i]->hdr;
					uint32 bodylen =hdr.bodylen;
					hdr.serialid = serial;
					msg_vec[i]->hdr.serialid=serial;
					serial++;
					uint32 sendlen = bodylen+sizeof(net_port_header_t);
					assert(sendlen<1024);
					transfer_out(hdr);
					memcpy(send_buf, (char*)&hdr,sizeof(net_port_header_t));
					if(bodylen >0) memcpy(send_buf+sizeof(net_port_header_t), msg_vec[i]->body, bodylen);
					if (!proxy->_socket_fd.is_vaild() || st_write(proxy->_socket_fd.get_fd(), send_buf , sendlen,SEC2USEC(REQUEST_TIMEOUT))<0)
					{
					//	printf("%s:%d -> %d\n",__FUNCTION__,__LINE__, st_thread_self());
						exit = true;
					}
					
				}
				msg_vec[i]->release();
			}
			msg_vec.clear();
			if(exit) break;
		}
	}
	close(proxy);
	return NULL;
}

void pu_proxy_t::close(pu_proxy_t* proxy)
{
	if(proxy->_socket_fd.is_vaild()) 
	{
		proxy->_socket_fd.uninit();
		proxy->notify();
	}

	{
	
	/*	proxy->_msg_queue.clear();
		
		for(uint i = 0; i<proxy->_wait_queue.size();i++)
		{
			net_port_msg_t* msg = proxy->_wait_queue[i];
			if(msg->cond) st_cond_signal(msg->cond);
		}
		proxy->_wait_queue.clear();*/
	}

	//assert(proxy->_wait_queue.size() == 0);
	proxy->release();
}

void* pu_proxy_t::handle_recv(void*arg)
{
	pu_proxy_t* proxy = (pu_proxy_t*)arg;
	bool exit = false;
	while (1)
	{
		net_port_header_t hdr ={};
		
		if (!proxy->_socket_fd.is_vaild() || st_read(proxy->_socket_fd.get_fd(), &hdr, sizeof(net_port_header_t), SEC2USEC(REQUEST_TIMEOUT)) < 0)
		{
			printf("%s:%d -> %d\n",__FUNCTION__,__LINE__, st_thread_self());
			exit = true;
			break;
		}

		transfer_in(hdr);
	
		if(hdr.bodylen>0)
		{
			
			char*body = 0;
			if(hdr.bodylen>1024*1024) 
			{
				exit = true;
				break;
			}
			body = (char*)calloc(1, hdr.bodylen+1);
			if (!proxy->_socket_fd.is_vaild()||st_read_fully(proxy->_socket_fd.get_fd(), body, hdr.bodylen, SEC2USEC(REQUEST_TIMEOUT))<0)
			{
				printf("%s:%d\n",__FUNCTION__,__LINE__);
				exit = true;
				break;
			}
			hdr.cmdno &= 0x0000ffff;
			if (hdr.cmdno == hmcmd_recv_video)
			{
				net_port_msg_t* msg = new net_port_msg_t;
				msg->hdr = hdr;
				msg->body = body;
				for (uint i = 0; i<proxy->_pu_video_proxy_vec.size(); i++)
				{
					msg->add();
					proxy->_pu_video_proxy_vec[i]->send_msg(msg);
				}
				msg->release();
			}
			else if (hdr.cmdno == hmcmd_open_video)
			{
				proxy->_real_video_cmd->hdr.cmdno = hdr.cmdno;
				if (proxy->_real_video_cmd->body)
					free(proxy->_real_video_cmd->body);

				proxy->_real_video_cmd->hdr.bodylen = hdr.bodylen;
				proxy->_real_video_cmd->body = body;
			}
		}



		{

#if 0
			std::vector<net_port_msg_t*>::iterator iter;
			for(iter = proxy->_wait_queue.begin(); iter != proxy->_wait_queue.end(); ++iter)
			{
				if((*iter)->hdr.serialid == hdr.serialid)
				{
					net_port_msg_t* msg = (*iter);
					msg->hdr.cmdno = hdr.cmdno;
					msg->hdr.bodylen=hdr.bodylen;

					if(hdr.bodylen>0)
					{
						msg->body = (char*)malloc(hdr.bodylen);
						memcpy((*iter)->body, body, hdr.bodylen);
					}
					iter = proxy->_wait_queue.erase(iter); 

					st_cond_signal(msg->cond);
					if(msg->hdr.cmdno==hmcmd_open_video)
					{
						proxy->_real_video_cmd->hdr.cmdno = hdr.cmdno;
						if(proxy->_real_video_cmd->body)
							free(proxy->_real_video_cmd->body);

						proxy->_real_video_cmd->hdr.bodylen=hdr.bodylen;
						proxy->_real_video_cmd->body = (char*)malloc(hdr.bodylen);
						memcpy(proxy->_real_video_cmd->body, body, hdr.bodylen);
					}
					break;
				}
			}
#endif
		
		}
		
		
	}
	close(proxy);
	return NULL;
}

void* pu_proxy_t::heart_thread(void* arg)
{
	pu_proxy_t* proxy = (pu_proxy_t*)arg;
	st_thread_create(handle_open_video, arg, 0,0);
	proxy->add();
	while(1)
	{
		uint32 heart_count  = 0;
		bool flag = false;
		while(heart_count<=10)
		{
			heart_count++;
			st_sleep(1);
			if(!proxy->_socket_fd.is_vaild())
			{
				flag = true;
				break;
			}
		}

		if(flag) break;
		{
			net_port_msg_t* msg = new net_port_msg_t;
			net_port_header_t hdr;
			hdr.cmdno = hmcmd_heartbeat;
			hdr.bodylen = 0;
			hdr.errcode = 0;
			msg->hdr = hdr;
			//msg->cond = st_cond_new();
			proxy->send_msg(msg);
		}
	}

	proxy->release();
	return NULL;
}

void* pu_proxy_t::handle_open_video(void*arg)
{
	pu_proxy_t* proxy = (pu_proxy_t*)arg;
	proxy->add();
	proxy->_real_video_cmd = new net_port_msg_t;
	char body[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<Message>"
		"<Channel>0</Channel>"
		"<StreamType>1</StreamType>"
		"<VideoType>1</VideoType>"
		"</Message>";
	proxy->_real_video_cmd->hdr.cmdno = hmcmd_open_video;
	proxy->_real_video_cmd->body = (char*)malloc(strlen(body)+1);
	memcpy(proxy->_real_video_cmd->body, body, strlen(body)+1);
	proxy->_real_video_cmd->hdr.bodylen = strlen(body)+1;
	proxy->_real_video_cmd->hdr.errcode = 0;
	//proxy->_real_video_cmd->cond = st_cond_new();
	proxy->_real_video_cmd->add();
	proxy->send_msg(proxy->_real_video_cmd);

	proxy->release();
	return NULL;
}

void pu_proxy_t::run(st_netfd_t cli_fd)
{
	_socket_fd.init(cli_fd);
	add();
	add();
	st_thread_create(handle_recv, this, 0,0);
	st_thread_create(handle_send, this, 0,0);
	
}