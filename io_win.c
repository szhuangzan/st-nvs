/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Portions created by SGI are Copyright (C) 2000 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Silicon Graphics, Inc.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the ____ license (the  "[____] License"), in which case the provisions
 * of [____] License are applicable instead of those above. If you wish to
 * allow use of your version of this file only under the terms of the [____]
 * License and not to allow others to use your version of this file under the
 * NPL, indicate your decision by deleting  the provisions above and replace
 * them with the notice and other provisions required by the [____] License.
 * If you do not delete the provisions above, a recipient may use your version
 * of this file under either the NPL or the [____] License.
 */

/*
 * This file is derived directly from Netscape Communications Corporation,
 * and consists of extensive modifications made during the year(s) 1999-2000.
 */

#include <stdlib.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "common.h"


extern DWORD WINAPI _st_iocp_thread_pool(LPVOID* param);

#if EAGAIN != EWOULDBLOCK
#define _IO_NOT_READY_ERROR  ((errno == EAGAIN) || (errno == EWOULDBLOCK))
#else
#define _IO_NOT_READY_ERROR  (errno == EAGAIN)
#endif
#define _LOCAL_MAXIOV  16

/* Winsock Data */
static WSADATA wsadata;

// 完成端口
HANDLE  _st_comletion_port;
st_queue_t*	_st_lock_free_queue;
st_fifo_t*   _st_fifo;

/* File descriptor object free list */
static st_netfd_t *_st_netfd_freelist = NULL;

static void _st_netfd_free_aux_data(st_netfd_t *fd);

APIEXPORT int st_errno(void)
 {
  return(errno);
 }

/* _st_GetError xlate winsock errors to unix */
int _st_GetError(int err)
 {
  int syserr;

  if(err == 0) syserr=GetLastError();
  SetLastError(0);
  if(syserr < WSABASEERR) return(syserr);
  switch(syserr)
   {
    case WSAEINTR:   
         break;          
    case WSAEBADF:   syserr=EBADF;
                     break;
    case WSAEACCES:  syserr=EACCES;
                     break;
    case WSAEFAULT:  syserr=EFAULT;
                     break;
    case WSAEINVAL:  syserr=EINVAL;
                     break;
    case WSAEMFILE:  syserr=EMFILE;
                     break;
    case WSAEWOULDBLOCK:  syserr=EAGAIN;
                     break;
    case WSAEINPROGRESS:  syserr=EINTR;
                     break;
    case WSAEALREADY:  syserr=EINTR;
                     break;
    case WSAENOTSOCK:  syserr=ENOTSOCK;
                     break;
    case WSAEDESTADDRREQ: syserr=EDESTADDRREQ;
                     break;
    case WSAEMSGSIZE: syserr=EMSGSIZE;
                     break;
    case WSAEPROTOTYPE: syserr=EPROTOTYPE;
                     break;
    case WSAENOPROTOOPT: syserr=ENOPROTOOPT;
                     break;
    case WSAEOPNOTSUPP: syserr=EOPNOTSUPP;
                     break;
    case WSAEADDRINUSE: syserr=EADDRINUSE;
                     break;
    case WSAEADDRNOTAVAIL: syserr=EADDRNOTAVAIL;
                     break;
    case WSAECONNABORTED: syserr=ECONNABORTED;
                     break;
    case WSAECONNRESET: syserr=ECONNRESET;
                     break;
    case WSAEISCONN: syserr=EISCONN;
                     break;
    case WSAENOTCONN: syserr=ENOTCONN;
                     break;
    case WSAETIMEDOUT: syserr=ETIMEDOUT;
                     break;
    case WSAECONNREFUSED: syserr=ECONNREFUSED;
                     break;
    case WSAEHOSTUNREACH: syserr=EHOSTUNREACH;
                     break;
   }
  return(syserr);
 }



/* getpagesize */
size_t getpagesize(void)
 {
  SYSTEM_INFO sysinf;
  GetSystemInfo(&sysinf);
  return(sysinf.dwPageSize);
 }

int _st_io_init(void)
{
	int result = 0;
	unsigned i = 0;
	SYSTEM_INFO info;

    WSAStartup(2,&wsadata);

	
	_st_comletion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0); //创建一个IO完成端口

	if(!_st_comletion_port)
	{
		result = GetLastError();
	}

	//创建线程池
	GetSystemInfo(&info);  

	for(i=0; i<info.dwNumberOfProcessors*2; i++)
	{
		HANDLE thread = CreateThread(NULL,0,_st_iocp_thread_pool, NULL, 0, NULL);
		CloseHandle(thread);
	}

	_st_lock_free_queue = _st_lock_free_queue_open();
	_st_fifo = (st_fifo_t*)calloc(1, sizeof(st_fifo_t));
	 return result;
}


APIEXPORT int st_getfdlimit(void)
{
   assert(false);
   return 0;
}


APIEXPORT void st_netfd_free(st_netfd_t *fd)
{
  if (!fd->inuse)
    return;

  fd->inuse = 0;
  if (fd->aux_data)
    _st_netfd_free_aux_data(fd);
  if (fd->private_data && fd->destructor)
    (*(fd->destructor))(fd->private_data);
  fd->private_data = NULL;
  fd->destructor = NULL;
  fd->next = _st_netfd_freelist;
  _st_netfd_freelist = fd;
}


static st_netfd_t *_st_netfd_new(int osfd, int nonblock, int is_socket)
{
  st_netfd_t *fd = 0;


  if (_st_netfd_freelist) {
    fd = _st_netfd_freelist;
    _st_netfd_freelist = _st_netfd_freelist->next;
  } else 
  {
	  fd = (st_netfd_t*)calloc(1, sizeof(st_netfd_t));
	  fd->osfd = osfd;
    if (!fd)
      return NULL;
  }


  fd->inuse = 1;
  fd->next = NULL;

  if(is_socket == FALSE) return(fd);

  return fd;
}

APIEXPORT st_netfd_t* st_netfd_listen(struct sockaddr_in addr)
{
	int n = 0;
	st_netfd_t* fd;
	st_per_handle_data* per_handle_data = NULL;
	//创建一个监听套接字(进行重叠操作)
	SOCKET Listen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (setsockopt(Listen, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0)
	{
		assert(0);
	}
	//将监听套接字与完成端口绑定
	fd = _st_netfd_new(Listen, 1, 1);
	
	per_handle_data = (st_per_handle_data*)GlobalAlloc(GPTR, sizeof(st_per_handle_data));
	per_handle_data->socket = Listen;
	CreateIoCompletionPort((HANDLE)Listen, _st_comletion_port, (ULONG_PTR)per_handle_data, 0);
	bind(Listen, (PSOCKADDR)&addr, sizeof(addr));
	listen(Listen, 10);
	return fd;
}

APIEXPORT st_netfd_t *st_netfd_open(int osfd)
{
  return _st_netfd_new(osfd, 1, 0);
}


APIEXPORT st_netfd_t *st_netfd_open_socket(int osfd)
{
  return _st_netfd_new(osfd, 1, 1);
}


APIEXPORT int st_netfd_close(st_netfd_t *fd)
{
  st_netfd_free(fd);
  closesocket(fd->osfd);
  errno=_st_GetError(0);
  return(errno);
}


APIEXPORT int st_netfd_fileno(st_netfd_t *fd)
{
	return fd->osfd;
}


APIEXPORT void st_netfd_setspecific(st_netfd_t *fd, void *value,
                                    st_destructor_t destructor)
{
  if (value != fd->private_data) {
    /* Free up previously set non-NULL data value */
    if (fd->private_data && fd->destructor)
      (*(fd->destructor))(fd->private_data);
  }
  fd->private_data = value;
  fd->destructor = destructor;
}


APIEXPORT void *st_netfd_getspecific(st_netfd_t *fd)
{
  return (fd->private_data);
}


/*
 * Wait for I/O on a single descriptor.
 */
APIEXPORT int st_netfd_poll(st_netfd_t *fd, int how, st_utime_t timeout)
{
	assert(0);
	return -1; 
}


/* No-op */
int st_netfd_serialize_accept(st_netfd_t *fd)
{
  fd->aux_data = NULL;
  return 0;
}

/* No-op */
static void _st_netfd_free_aux_data(st_netfd_t *fd)
{
	fd->aux_data = NULL;
}

APIEXPORT st_netfd_t * st_accept(st_netfd_t *fd, char* recv_buf, int recv_len)
{

	int rc = 0;
	st_netfd_t* newfd = NULL;
	SOCKET osfd = -1;

	st_thread_t* thread = _ST_CURRENT_THREAD();
	st_context_switch_t* context = thread->context_switch;

	LPFN_ACCEPTEX lpfnAcceptEx = NULL;     //AcceptEx函数指针
	//Accept function GUID
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	//get acceptex function pointer
	DWORD dwBytes = 0;

	assert(recv_buf);
	assert(recv_len);

	if (WSAIoctl(fd->osfd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL) == 0)
	{
	}
	else{
		switch(WSAGetLastError())
		{
		case WSAENETDOWN:
		
			break;
		case WSAEFAULT:
			
			break;
		case WSAEINVAL:
			
			break;
		case WSAEINPROGRESS:
			
			break;
		case WSAENOTSOCK:
			
			break;
		case WSAEOPNOTSUPP:
			
			break;
		case WSA_IO_PENDING:
			
			break;
		case WSAEWOULDBLOCK:
		
			break;
		case WSAENOPROTOOPT:
		
			break;
		}
		return NULL;
	}

	 osfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	 newfd = _st_netfd_new(osfd, 1, 1);
	//在使用AcceptEx前需要事先重建一个套接字用于其第二个参数。这样目的是节省时间
	//通常可以创建一个套接字库
	
	 context->buffer.buf = recv_buf;
	 context->buffer.len = recv_len;

	//调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
	//注意这里使用了重叠模型，该函数的完成将在与完成端口关联的工作线程中处理
	 context->osfd = osfd;
	 context->operator_type = ACCEPT_OPER;
	lpfnAcceptEx(fd->osfd, newfd->osfd, recv_buf,
		recv_len- ((sizeof(SOCKADDR_IN)+16) * 2),
		sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,&dwBytes,
		&(thread->context_switch->overlapped));
	if(rc == FALSE)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf("%s:%d - > %d\n",__FUNCTION__,__LINE__,WSAGetLastError());
			return NULL;
		}
	}
	_st_wait(newfd, ST_UTIME_NO_TIMEOUT);
	return newfd;
}


APIEXPORT st_netfd_t* st_connect(struct sockaddr_in addr, int addrlen, st_utime_t timeout)
{

	int rc = 0;
	st_per_handle_data* per_handle_data = NULL;
	st_netfd_t* newfd = NULL;
	SOCKET osfd = -1;

	st_thread_t* thread = _ST_CURRENT_THREAD();
	st_context_switch_t* context = thread->context_switch;

	LPFN_CONNECTEX lpfnConnectEx = NULL;     //AcceptEx函数指针
	//Accept function GUID
	GUID guidConnectEx = WSAID_CONNECTEX;
	//get acceptex function pointer
	DWORD dwBytes = 0;

	osfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	newfd = _st_netfd_new(osfd, 1, 1);

	if (WSAIoctl(newfd->osfd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof(guidConnectEx), &lpfnConnectEx, sizeof(lpfnConnectEx),
		&dwBytes, NULL, NULL) == 0)
	{
	}
	else{
		switch(WSAGetLastError())
		{
		case WSAENETDOWN:

			break;
		case WSAEFAULT:

			break;
		case WSAEINVAL:

			break;
		case WSAEINPROGRESS:

			break;
		case WSAENOTSOCK:

			break;
		case WSAEOPNOTSUPP:

			break;
		case WSA_IO_PENDING:

			break;
		case WSAEWOULDBLOCK:

			break;
		case WSAENOPROTOOPT:

			break;
		}
		return NULL;
	}


	//在使用AcceptEx前需要事先重建一个套接字用于其第二个参数。这样目的是节省时间
	//通常可以创建一个套接字库


	//调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
	//注意这里使用了重叠模型，该函数的完成将在与完成端口关联的工作线程中处理
	context->osfd = osfd;
	context->operator_type = CONNECT_OPER;

	{
		SOCKADDR_IN temp;
		
		per_handle_data = (st_per_handle_data*)GlobalAlloc(GPTR, sizeof(st_per_handle_data));
		per_handle_data->socket = osfd;
		
		temp.sin_family = AF_INET;
		temp.sin_port = htons(0);
		temp.sin_addr.s_addr = htonl(ADDR_ANY);
		bind(osfd, (SOCKADDR_IN*)&temp, sizeof(SOCKADDR));
		CreateIoCompletionPort((HANDLE)osfd, _st_comletion_port, (ULONG_PTR)per_handle_data, 0);
	}

	rc = lpfnConnectEx(osfd, (SOCKADDR_IN*)&addr, addrlen, 0, 0, 0, (OVERLAPPED *)&context->overlapped);
	if(rc == FALSE)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			GlobalFree(per_handle_data);
			printf("%s:%d - > %d\n",__FUNCTION__,__LINE__,WSAGetLastError());
			return NULL;
		}
	}
	_st_wait(newfd, ST_UTIME_NO_TIMEOUT);
	return newfd;
}

#include<stdio.h>
APIEXPORT ssize_t st_read(st_netfd_t *fd, void *buf, size_t nbyte, st_utime_t timeout)
{
  ssize_t n = 0;
  DWORD recv_bytes = 0;
  DWORD flags = 0;
  int rc = 0;
 
  st_thread_t* thread = _ST_CURRENT_THREAD();
  st_context_switch_t* context = thread->context_switch;

  context->operator_type = RECV_OPER;
  context->buffer.buf = buf;
  context->buffer.len = nbyte;
 
  rc = WSARecv(fd->osfd, &(context->buffer), 1,
	  &recv_bytes, &flags, &(thread->context_switch->overlapped), NULL);
  if (rc != 0)
  {
	  if (WSAGetLastError() != ERROR_IO_PENDING)
	  {
		  printf("%s:%d ->%d   %x\n", __FUNCTION__, __LINE__, WSAGetLastError(),st_thread_self());
		  return -1;
	  }
  }
  _st_wait(fd, timeout);
  return context->valid_len;
}


APIEXPORT ssize_t st_read_fully(st_netfd_t *fd, void *buf, size_t nbyte,
                                st_utime_t timeout)
{
  size_t nleft = nbyte;
  int n = 0;
//  printf("%s:%d    %x\n",__FUNCTION__,__LINE__,st_thread_self());	
  while (nleft > 0)
  {
	 //  printf("%s:%d    %x\n",__FUNCTION__,__LINE__,st_thread_self());		
	  if ((n = st_read(fd, buf, nleft, timeout)) < 0)
	{
      errno=_st_GetError(0);
      if (errno == EINTR)
        continue;
        return -1;
    } else {
		
			
	//  printf("1 %s:%d nleft = %d, n= %d         %x\n",__FUNCTION__,__LINE__, nleft, n, st_thread_self());	
      nleft -= n;
	//  printf("2 %s:%d nleft = %d, n= %d          %x\n",__FUNCTION__,__LINE__, nleft, n,st_thread_self());	
      if (nleft == 0 || n == 0)
        break;
	 // printf("3 %s:%d nleft = %d, n= %d          %x\n",__FUNCTION__,__LINE__, nleft, n,st_thread_self());	
      buf = (void *)((char *)buf + n);
    }
  }
  return (ssize_t)(nbyte - nleft);
}

APIEXPORT void st_get_addr(st_netfd_t* fd, char*local_addr, char* remote_addr)
{
	//使用GetAcceptExSockaddrs函数 获得具体的各个地址参数.
	 char buf[128] ;
	//取本地和客户端地址
	 LPFN_GETACCEPTEXSOCKADDRS lpGetAcceptExSockAddr;
	 GUID GUIDGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	 DWORD dwResult;
	 int nResult = WSAIoctl(
	  fd->osfd,
	  SIO_GET_EXTENSION_FUNCTION_POINTER,
	  &GUIDGetAcceptExSockAddrs,
	  sizeof(GUID),
	  &lpGetAcceptExSockAddr,
	  sizeof(lpGetAcceptExSockAddr),
	  &dwResult,
	  NULL,
	  NULL
	  );
	 //
	 int nLoal, nRemote;
	 DWORD dwAddrLen =
	  sizeof(SOCKADDR_IN)+16;
	 SOCKADDR_IN Addr;
	 SOCKADDR* pLocal_Addr;
	 SOCKADDR* pRemote_Addr;
	 lpGetAcceptExSockAddr(buf,
	  128 - 2 * dwAddrLen,
	  dwAddrLen,
	  dwAddrLen,
	  &pLocal_Addr,
	  &nLoal,
	  &pRemote_Addr,
	  &nRemote);
	 memcpy(&Addr, (const void*)pLocal_Addr, sizeof(SOCKADDR));
	// memcpy(&per_handler_data->clientAddr, (const void*)pRemote_Addr, sizeof(SOCKADDR));

}

APIEXPORT ssize_t st_write(st_netfd_t *fd, const char *buf, size_t nbyte,
                           st_utime_t timeout)
{
  ssize_t n = 0;
  int rc = 0;
  ssize_t nleft = nbyte;
  st_thread_t* thread = _ST_CURRENT_THREAD();
  st_context_switch_t* context = thread->context_switch;

  context->operator_type = SEND_OPER;
  context->buffer.buf = (char*)buf;
  context->buffer.len = nbyte;

  //调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
  //注意这里使用了重叠模型，该函数的完成将在与完成端口关联的工作线程中处理
 
  rc = WSASend(fd->osfd, &(context->buffer), 1, &n, 0, &(context->overlapped), NULL);
  if (rc != 0)
  {
	  if (WSAGetLastError() != ERROR_IO_PENDING)
	  {
		//  printf("%s:%d ->%d\n", __FUNCTION__, __LINE__, WSAGetLastError());
		  return -1;
	  }
  }
  _st_wait(fd, ST_UTIME_NO_TIMEOUT);

  return (ssize_t)nbyte;
}


APIEXPORT ssize_t st_writev(st_netfd_t *fd, const struct iovec *iov, int iov_size,
                            st_utime_t timeout)
{
 errno=EOPNOTSUPP;
 return(-1);
}


/*
 * Simple I/O functions for UDP.
 */
APIEXPORT int st_recvfrom(st_netfd_t *fd, void *buf, int len, struct sockaddr *from,
                          int *fromlen, st_utime_t timeout)
{
  int n = 0;
  return n;
}


APIEXPORT int st_sendto(st_netfd_t *fd, const void *msg, int len, struct sockaddr *to,
                        int tolen, st_utime_t timeout)
{
  int n = 0;

  return n;
}


/*
 * To open FIFOs or other special files.
 */
APIEXPORT st_netfd_t *st_open(const char *path, int oflags, mode_t mode)
{
  int osfd, err;
  st_netfd_t *newfd;

  while ((osfd = open(path, oflags, mode)) < 0) {
    errno=_st_GetError(0);
    if (errno != EINTR)
      return NULL;
  }

  newfd = _st_netfd_new(osfd, 0, 0);
  if (!newfd) {
    errno=_st_GetError(0);
    err = errno;
    close(osfd);
    errno = err;
  }

  return newfd;
}

APIEXPORT int st_wait(st_netfd_t *fd,  st_utime_t timeout)
{
 return(_st_wait(fd,timeout));
}
