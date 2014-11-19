#ifndef __CU_PROXY_HPP__
#define __CU_PROXY_HPP__
#include "proto.hpp"
#include "st.h"
#include <cstring>
#include <vector>
#include <malloc.h>
#include <vector>
#include "pu_proxy.hpp"

class cu_proxy_t:public server_base_t
{
	typedef struct _cu_msg_wrap_t
	{
		cu_proxy_t*		cu_proxy;
		net_port_msg_t* msg;
	}cu_msg_wrap_t;
public:
	cu_proxy_t();
	virtual ~cu_proxy_t();
	void init(pu_proxy_t* pu_proxy);
	void run(st_netfd_t cli_fd);
	void send_msg(net_port_msg_t* msg);
	void uninit()
	{
		_pu_proxy->release();
		_pu_proxy = 0;
	}
private:
	static void* handle_recv(void*arg);
	static void* handle_send(void*arg);
	static void* send_proxy(void*arg);
	static void* open_real_video(void*arg);
	static bool check_net(cu_proxy_t* cu)
	{
		if(!cu->_pu_proxy)
		{
			return false;
		}
		return cu->_pu_proxy->is_valid();
	}
	static void close(cu_proxy_t* cu);
private:
	msg_queue_t					 _msg_queue;
	socket_fd_t					 _socket_fd;
	pu_proxy_t*				     _pu_proxy;
	uint32						 _video_serial;
	cu_msg_wrap_t*				 _video_msg_wrap;
	static	uint32				 _cu_dev_count;
private:
	
};


#endif