#ifndef __SERVER_HPP__
#define __SERVER_HPP__
#include "proto.hpp"
#include "st.h"
#include <cstring>
#include <vector>
#include <malloc.h>
#include <string>
#include <map>
#include "pu_proxy.hpp"

#define SEC2USEC(s) ((s)*1000000LL)
/* Request read timeout (in seconds) */
#define REQUEST_TIMEOUT 60


class ServerHandler
{

typedef	struct socket_info
{
	st_netfd_t nfd;               /* Listening socket                     */
	char *addr;                   /* Bind address                         */
	unsigned int port;            /* Port                                 */
	int wait_threads;             /* Number of threads waiting to accept  */
	int busy_threads;             /* Number of threads processing request */
	int rqst_count;               /* Total number of processed requests   */

	uint32 totoal_threads()
	{
		return wait_threads+busy_threads;
	}

	void sign_thread(bool busy = true)
	{
		if(busy)
		{
			busy_threads++;
			wait_threads--;
		}
		else
		{
			busy_threads--;
			wait_threads++;
		}
	}

	uint32 rqst_cout()
	{
		return rqst_count++;
	}
	bool wait_thread_is_empty()
	{
		return wait_threads == 0;
	}
	socket_info()
	{
		memset(this, 0, sizeof(socket_info));
	}
} srv_socket;



public:
	ServerHandler()
		:_socket_info(new srv_socket)
	{

	}
	void run();
	static void unregister(std::string sn)
	{
		{
			std::map<std::string,pu_proxy_t*>::iterator iter = _device_map.find(sn);
			if(iter!= _device_map.end())
			{
				ServerHandler::_device_map.erase(iter);
			}
		}

	}
	
private:
	st_netfd_t create_listeners();
	void set_thread_throttling();
	void start_threads(st_netfd_t);

	static char* read_auth_type_wrap(st_netfd_t fd);
	static bool write_auth_wrap(st_netfd_t fd);
	static void* handle_connections(void *arg);
	static void* handle_session(void*);
	static void* static_print(void*);

private:
	srv_socket*		_socket_info;
	static std::map<std::string,pu_proxy_t*>    _device_map;
	uint32			_login_count;

};

#endif