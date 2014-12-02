#ifndef __PU_PROXY_HPP__
#define __PU_PROXY_HPP__
#include "proto.hpp"
#include "st.h"
#include <cstring>
#include <vector>
#include <malloc.h>
#include <vector>
class cu_proxy_t;
typedef struct msg_queue_t
{
	std::vector<net_port_msg_t*>	_msg_vec;
	msg_queue_t()
	{
		_msg_vec.clear();
	}

	void push(net_port_msg_t* msg)
	{
		msg->add();
		_msg_vec.push_back(msg);
	}

	void backup(std::vector<net_port_msg_t*>& vec)
	{
		for(uint i=0;i<_msg_vec.size();i++)
		{
			vec.push_back(_msg_vec[i]);
		}

		clear();
	}

	void clear()
	{
		for(uint i=0;i<_msg_vec.size();i++)
		{
			_msg_vec[i]->release();
		}
		_msg_vec.clear();
	}

	std::size_t size()
	{
		return _msg_vec.size();
	}

	~msg_queue_t()
	{
		for(uint i=0;i<_msg_vec.size();i++)
		{
			_msg_vec[i]->release();
		}
		_msg_vec.clear();
	}

}msg_queue_t;


	
class pu_proxy_t : public server_base_t
{
public:
	static	uint32					 _pu_dev_count;
	pu_proxy_t(std::string sn);
	virtual ~pu_proxy_t();
	void init();
	bool is_video_open()
	{
		return _real_video_cmd!=0;
	}
	void run(st_netfd_t cli_fd);
	void send_msg(net_port_msg_t* msg);
	net_port_msg_t* open_real_video(cu_proxy_t*, net_port_msg_t*);
	void close_real_video(cu_proxy_t*);
	bool is_valid()
	{
		return _socket_fd.is_vaild();
	}
private:
	static void* heart_thread(void*arg);
	static void* handle_recv(void*arg);
	static void* handle_send(void*arg);
	static void* handle_open_video(void*);
	static void  close(pu_proxy_t*);
private:
	std::string						 _sn;
	msg_queue_t						 _msg_queue;
	std::vector<net_port_msg_t*>	_wait_queue;
	socket_fd_t						_socket_fd;
	std::vector<cu_proxy_t*>		 _pu_video_proxy_vec;
	net_port_msg_t*					 _real_video_cmd;
	
	
};


#endif