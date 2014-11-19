#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include<process.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <WinSock.h>
#include <errno.h>
#include <cassert>
#include <algorithm>
#include <iostream>

#include "md.h"
#include "st.h"
#include "server.hpp"
#include "proto.hpp"
#include "xml.hpp"
#include "cu_proxy.hpp"

#define SERV_PORT_DEFAULT 10000
#define LISTENQ_SIZE_DEFAULT 512
#define ST_UTIME_NO_TIMEOUT (-1)


/* Max number of "spare" threads per process per socket */
#define MAX_WAIT_THREADS_DEFAULT 16
/* Number of file descriptors needed to handle one client session */
#define FD_PER_THREAD 2

/* Access log buffer flushing interval (in seconds) */
#define ACCLOG_FLUSH_INTERVAL 30


/*
* Thread throttling parameters (all numbers are per listening socket).
* Zero values mean use default.
*/
static int max_threads = 0;       /* Max number of threads         */
static int max_wait_threads = 0;  /* Max number of "spare" threads */
static int min_wait_threads = 2;  /* Min number of "spare" threads */
static int vp_count = 1;


std::map<std::string,pu_proxy_t*> ServerHandler::_device_map;

void ServerHandler::create_listeners()
{
	int n = 0, sock;
	struct sockaddr_in serv_addr;
	struct hostent *hp = 0;
	unsigned short port = 0;

	if (_socket_info->addr== '\0')
		_socket_info->addr = strdup("0.0.0.0");
	if (port == 0)
		port = SERV_PORT_DEFAULT;
	_socket_info->port=port;

	/* Create server socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		assert(0);
	}

	n = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0)
	{
		assert(0);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(_socket_info->addr);
	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		/* not dotted-decimal */
		if ((hp = gethostbyname(_socket_info->addr)) == NULL)
		{
			assert(0);
		}
		memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
	}
	_socket_info->port = port;

	/* Do bind and listen */
	if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		assert(0);
	}
	if (listen(sock, LISTENQ_SIZE_DEFAULT) < 0)
	{
		assert(0);
	}

	/* Create file descriptor object from OS socket */
	if ((_socket_info->nfd = st_netfd_open_socket(sock)) == NULL)
	{
		assert(0);
	}
	/*
	* On some platforms (e.g. IRIX, Linux) accept() serialization is never
	* needed for any OS version.  In that case st_netfd_serialize_accept()
	* is just a no-op. Also see the comment above.
	*/
	if (st_netfd_serialize_accept(_socket_info->nfd) < 0)
	{
		assert(0);
	}

}

void ServerHandler::set_thread_throttling()
{
	///*
	//* Calculate total values across all processes.
	//* All numbers are per listening socket.
	//*/
	//if (max_wait_threads == 0)
	//	max_wait_threads = MAX_WAIT_THREADS_DEFAULT * vp_count;
	///* Assuming that each client session needs FD_PER_THREAD file descriptors */
	//if (max_threads == 0)
	//	max_threads = (st_getfdlimit() * vp_count) / FD_PER_THREAD ;
	//if (max_wait_threads > max_threads)
	//	max_wait_threads = max_threads;

	///*
	//* Now calculate per-process values.
	//*/
	//if (max_wait_threads % vp_count)
	//	max_wait_threads = max_wait_threads / vp_count + 1;
	//else
	//	max_wait_threads = max_wait_threads / vp_count;
	//if (max_threads % vp_count)
	//	max_threads = max_threads / vp_count + 1;
	//else
	//	max_threads = max_threads / vp_count;

	//if (min_wait_threads > max_wait_threads)
	//	min_wait_threads = max_wait_threads;
}

void ServerHandler::start_threads()
{

	st_thread_create(handle_connections, (void *)_socket_info, 0, 0);
	//int n = 0;
	//for (n = 0; n < max_wait_threads; n++) {
	//	if (st_thread_create(handle_connections, (void *)_socket_info, 0, 0) != NULL)
	//	{
	//		_socket_info->sign_thread(false);
	//	}
	//}
	//if (_socket_info->wait_thread_is_empty())
	//	exit(1);
}


static void parseXML(char* buf)
{
	
}

void* ServerHandler::handle_connections(void *arg)
{
	st_netfd_t srv_nfd = 0;
	st_netfd_t cli_nfd = 0;
	struct sockaddr_in from = {};
	int fromlen = sizeof(from);

	srv_socket* _socket_info = (srv_socket*)arg;
	srv_nfd = _socket_info->nfd;

	{
		cli_nfd = st_accept(srv_nfd, (struct sockaddr *)&from, &fromlen,ST_UTIME_NO_TIMEOUT);
		if (cli_nfd == NULL) 
		{
			
			printf("%s:%d -> %d\n", __FUNCTION__,__LINE__, st_errno());
			st_thread_create(handle_connections, (void *)_socket_info, 0, 0);
			return NULL;
		
		}
		{
			char ip[20] ;
			memset(ip,0,20);
			sprintf(ip,"%d:%d:%d:%d",from.sin_addr.S_un.S_un_b.s_b1, 
				from.sin_addr.S_un.S_un_b.s_b2, 
				from.sin_addr.S_un.S_un_b.s_b3, 
				from.sin_addr.S_un.S_un_b.s_b4); 

		//	printf("connect IP:PORT -> %s:%d\n",ip,from.sin_port);

		}
	
		st_thread_create(handle_session,cli_nfd,0,0);
		st_thread_create(handle_connections, (void *)_socket_info, 0, 0);
	
	}
	return NULL;
}

#if 0
void* ServerHandler::handle_connections(void *arg)
{
	st_netfd_t srv_nfd = 0;
	st_netfd_t cli_nfd = 0;
	struct sockaddr_in from = {};
	int fromlen = sizeof(from);

	srv_socket* _socket_info = (srv_socket*)arg;
	srv_nfd = _socket_info->nfd;

	while (_socket_info->wait_threads <= max_wait_threads) 
	{
		cli_nfd = st_accept(srv_nfd, (struct sockaddr *)&from, &fromlen,
			ST_UTIME_NO_TIMEOUT);
		if (cli_nfd == NULL) 
		{
			{
				assert(0);
			}
			continue;
		}
		{
			char ip[20] ;
			memset(ip,0,20);
			sprintf(ip,"%d:%d:%d:%d",from.sin_addr.S_un.S_un_b.s_b1, 
				from.sin_addr.S_un.S_un_b.s_b2, 
				from.sin_addr.S_un.S_un_b.s_b3, 
				from.sin_addr.S_un.S_un_b.s_b4); 

			printf("connect IP:PORT -> %s:%d\n",ip,from.sin_port);

		}

		/* Save peer address, so we can retrieve it later */
		_socket_info->sign_thread();

		if (_socket_info->wait_threads < min_wait_threads && _socket_info->totoal_threads()< max_threads) {
			/* Create another spare thread */
			if (st_thread_create(handle_connections, (void *)_socket_info, 0, 0) != NULL)
				_socket_info->sign_thread(false);
			else
			{
				assert(0);
			}
		}
		//handle_session(cli_nfd);
		st_thread_create(handle_session,cli_nfd,0,0);
		_socket_info->sign_thread(false);
	}
	_socket_info->sign_thread(false);
	return NULL;
}
#endif

void* ServerHandler::handle_session(void* arg)
{
	st_netfd_t cli_fd = (st_netfd_t)arg;
	char* sn = 0;
	char* ptr = read_auth_type_wrap(cli_fd);
	if(ptr)
	{
		DMXml xml;
		xml.Decode(ptr);

		char* type = xml.GetRoot()->FindElement("Type")->GetValueText();
		if(!type)
		{
			st_netfd_close(cli_fd);
			free(ptr);
			return NULL;

		}
		sn = xml.GetParent()->FindElement("SN")->GetValueText();
		if(!sn) sn = xml.GetRoot()->FindElement("Sn")->GetValueText();
		if(!strcmp(type, "\"10\""))
		{
			printf("SN: %s Login\n", sn);
			if(_device_map.find(sn) == _device_map.end())
			{
				_device_map[sn] = new pu_proxy_t(sn);
			}
			else
			{
				return NULL;
			}
			assert(sn);
			write_auth_wrap(cli_fd);
			_device_map[sn]->init();
			_device_map[sn]->run(cli_fd);
			_device_map[sn]->release();
		}
		else if(!strcmp(type, "10"))
		{
			char sn_tmp[20] = {};
			
			sprintf(sn_tmp, "\"%s\"",sn);
			if(_device_map.find(sn_tmp) == _device_map.end())
			{
				printf("SN: %s Login\n", sn);
				_device_map[sn_tmp] = new pu_proxy_t(sn_tmp);
			}
			else
			{
				return NULL;
			}
			assert(sn_tmp);
			write_auth_wrap(cli_fd);
			_device_map[sn_tmp]->init();
			_device_map[sn_tmp]->run(cli_fd);
			_device_map[sn_tmp]->release();
		}
		else
		{
			char sn_tmp[20] = {};
			sprintf(sn_tmp, "\"%s\"",sn);
			if(_device_map.find(sn_tmp)==_device_map.end())
			{
				st_netfd_close(cli_fd);
				free(ptr);
				return NULL;
			}

			pu_proxy_t* pu_proxy = _device_map[sn_tmp];
			if(!pu_proxy->is_video_open())
			{
				st_netfd_close(cli_fd);
				free(ptr);
				return NULL;
			}
			
			if(!write_auth_wrap(cli_fd))
			{
				st_netfd_close(cli_fd);
				free(ptr);
				return NULL;
			}
			
			cu_proxy_t* cu_proxy = new cu_proxy_t();
			cu_proxy->init(pu_proxy);
			cu_proxy->run(cli_fd);
			cu_proxy->release();
			
		}
		free(ptr);
	}

	 return (0);
}
char* ServerHandler::read_auth_type_wrap(st_netfd_t fd)
{
	char* ptr = 0;
	// ¼øÈ¨£º

	net_port_header_t hdr ={};
	if (st_read_fully(fd, &hdr, sizeof(net_port_header_t)-4, SEC2USEC(REQUEST_TIMEOUT)) < 0) {
		st_netfd_close(fd);
		return NULL;
	}
	
	transfer_in(hdr);

	ptr = (char*)malloc(hdr.bodylen+1);
	memset(ptr, 0, hdr.bodylen+1);
	{
		if (st_read_fully(fd, ptr, hdr.bodylen, SEC2USEC(REQUEST_TIMEOUT)) < 0) {
			st_netfd_close(fd);
			return NULL;
		}
		ptr[hdr.bodylen]='\0';
	}
	
	return ptr;
}
 bool ServerHandler::write_auth_wrap(st_netfd_t fd)
{
	char buf[1024] = {};
	net_port_header_t hdr = {};
	hdr.cmdno = hmcmd_authencate;

	char body[512]="<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		"<Message>"
		"<UserName>\"guest\"</UserName >"
		"<UserType>1</UserType >"
		"<Ver>V2.0</Ver>"
		"</Message>";
	hdr.bodylen = strlen(body)+1;
	uint32 send_len = hdr.bodylen + sizeof(net_port_header_t)-4;
	transfer_out(hdr);
	memcpy(buf, &hdr, sizeof(hdr)-4);
	memcpy(buf+12, body, strlen(body)+1);
	if (st_write(fd, buf,send_len , ST_UTIME_NO_TIMEOUT) != send_len)
	{
		return false;
	}
	return true;

}

void ServerHandler::run()
{
	int rc = 0;

	/* Initialize the ST library */
	if (st_init() < 0)
		assert(0);

	printf("fd limit : %d\n", st_getfdlimit());
	/* Set thread throttling parameters */
	set_thread_throttling();

	/* Create listening sockets */
	create_listeners();

	/* Turn time caching on */
	st_timecache_set(1);
	start_threads();

	while(rc!=EINTR)
	{
		rc=st_sleep(-1);
		if(rc == -1) rc=errno;
	}

}