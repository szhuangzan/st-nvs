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

#if EAGAIN != EWOULDBLOCK
#define _IO_NOT_READY_ERROR  ((errno == EAGAIN) || (errno == EWOULDBLOCK))
#else
#define _IO_NOT_READY_ERROR  (errno == EAGAIN)
#endif
#define _LOCAL_MAXIOV  16

/* Winsock Data */
static WSADATA wsadata;

/* map winsock descriptors to small integers */
int fds[FD_SETSIZE+5];

/* File descriptor object free list */
static st_netfd_t *_st_netfd_freelist = NULL;
/* Maximum number of file descriptors that the process can open */
static int _st_osfd_limit = -1;

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
    case WSAEINTR:   syserr=EINTR;
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

/* freefdsslot */
int freefdsslot(void)
 {
  int i;
  for(i=5;i<FD_SETSIZE;i++)
   {
    if(fds[i] == 0) return(i);
   }
  return(-1);
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
 int i;
  /* Set maximum number of open file descriptors */
  _st_osfd_limit = FD_SETSIZE;
  WSAStartup(2,&wsadata);
  /* setup fds index. start at 5 */
  for(i=0;i<5;i++) fds[i]=-1;
  for(i=5;i<FD_SETSIZE;i++) fds[i]=0;
/* due to the braindead select implementation we need a dummy socket */
  fds[4]=socket(AF_INET,SOCK_STREAM,0);
  return 0;
}


APIEXPORT int st_getfdlimit(void)
{
  return _st_osfd_limit;
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
  st_netfd_t *fd;
  int flags = 1;
  int i;

  i=freefdsslot();
  if(i < 0)
   {
    errno=EMFILE;
    return(NULL);
   }
  fds[i]=osfd;  /* add osfd to index */

  if (_st_netfd_freelist) {
    fd = _st_netfd_freelist;
    _st_netfd_freelist = _st_netfd_freelist->next;
  } else {
    fd = calloc(1, sizeof(st_netfd_t));
    if (!fd)
      return NULL;
  }

  fd->osfd = i;
  fd->inuse = 1;
  fd->next = NULL;

  if(is_socket == FALSE) return(fd);
  if(nonblock) ioctlsocket(fds[fd->osfd], FIONBIO, &flags);

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
  closesocket(fds[fd->osfd]);
  fds[fd->osfd]=0;
  errno=_st_GetError(0);
  return(errno);
}


APIEXPORT int st_netfd_fileno(st_netfd_t *fd)
{
  return(fds[fd->osfd]);
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
  struct pollfd pd;
  int n;

  pd.fd = fd->osfd;
  pd.events = (short) how;
  pd.revents = 0;

  if ((n = _st_poll(&pd, 1, timeout,FALSE)) < 0)
    return -1;
  if (n == 0) {
    /* Timed out */
    errno = ETIME;
    return -1;
  }

  if(pd.revents == 0) {
    errno = EBADF;
    return -1;
  }

  return 0;
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

APIEXPORT st_netfd_t *st_accept(st_netfd_t *fd, struct sockaddr *addr, int *addrlen,
                                st_utime_t timeout)
{
  int osfd;
  st_netfd_t *newfd;

  while ((osfd = accept(fds[fd->osfd], addr, (socklen_t *)addrlen)) < 0)
   {
    errno=_st_GetError(0);
    if(errno == EINTR) continue;
    if(!_IO_NOT_READY_ERROR) return NULL;
    /* Wait until the socket becomes readable */
    if (st_netfd_poll(fd, POLLIN, timeout) < 0)
      return NULL;
   }
/* create newfd */
  newfd = _st_netfd_new(osfd, 1, 1);
  if(!newfd)
   {
    closesocket(osfd);
   }
  return newfd;
}

APIEXPORT int st_connect(st_netfd_t *fd, struct sockaddr *addr, int addrlen,
                         st_utime_t timeout)
{
  int n, err = 0;

  if(connect(fds[fd->osfd], addr, addrlen) < 0)
   {
    errno=_st_GetError(0);
    if( (errno != EAGAIN) && (errno != EINTR)) return(-1);
   /* Wait until the socket becomes writable */
    if(st_netfd_poll(fd, POLLOUT, timeout) < 0) return(-1);
      /* Try to find out whether the connection setup succeeded or failed */
    n = sizeof(int);
    if(getsockopt(fds[fd->osfd], SOL_SOCKET, SO_ERROR, (char *)&err,
                  (socklen_t *)&n) < 0) return(-1);
    if(err)
     {
      errno = _st_GetError(err);
      return -1;
     }
   }
  return(0);
}


APIEXPORT ssize_t st_read(st_netfd_t *fd, void *buf, size_t nbyte, st_utime_t timeout)
{
  ssize_t n;

  while((n = recv(fds[fd->osfd], buf, nbyte,0)) < 0)
  {
   errno=_st_GetError(0);
   if(errno == EINTR) continue;
   if(!_IO_NOT_READY_ERROR) return(-1);
    /* Wait until the socket becomes readable */
   if(st_netfd_poll(fd, POLLIN, timeout) < 0) return(-1);
  }
  return n;
}


APIEXPORT ssize_t st_read_fully(st_netfd_t *fd, void *buf, size_t nbyte,
                                st_utime_t timeout)
{
  ssize_t n;
  size_t nleft = nbyte;

  while (nleft > 0) {
    if ((n = recv(fds[fd->osfd], buf, nleft,0)) < 0) {
      errno=_st_GetError(0);
      if (errno == EINTR)
        continue;
      if (!_IO_NOT_READY_ERROR)
        return -1;
    } else {
      nleft -= n;
      if (nleft == 0 || n == 0)
        break;
      buf = (void *)((char *)buf + n);
    }
    /* Wait until the socket becomes readable */
    if (st_netfd_poll(fd, POLLIN, timeout) < 0)
      return -1;
  }

  return (ssize_t)(nbyte - nleft);
}


APIEXPORT ssize_t st_write(st_netfd_t *fd, const void *buf, size_t nbyte,
                           st_utime_t timeout)
{
  ssize_t n;
  ssize_t nleft = nbyte;

  while (nleft > 0) {
    if ((n = send(fds[fd->osfd], buf, nleft,0)) < 0) {
      errno=_st_GetError(0);
      if (errno == EINTR)
        continue;
      if (!_IO_NOT_READY_ERROR)
        return -1;
    } else {
      if (n == nleft)
        break;
      nleft -= n;
      buf = (const void *)((const char *)buf + n);
    }
    /* Wait until the socket becomes writable */
    if (st_netfd_poll(fd, POLLOUT, timeout) < 0)
      return -1;
  }

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
  int n;

  while ((n = recvfrom(fds[fd->osfd], buf, len, 0, from, (socklen_t *)fromlen))
         < 0) {
    errno=_st_GetError(0);
    if (errno == EINTR)
      continue;
    if (!_IO_NOT_READY_ERROR)
      return -1;
    /* Wait until the socket becomes readable */
    if (st_netfd_poll(fd, POLLIN, timeout) < 0)
      return -1;
  }

  return n;
}


APIEXPORT int st_sendto(st_netfd_t *fd, const void *msg, int len, struct sockaddr *to,
                        int tolen, st_utime_t timeout)
{
  int n;

  while ((n = sendto(fds[fd->osfd], msg, len, 0, to, tolen)) < 0) {
    errno=_st_GetError(0);
    if (errno == EINTR)
      continue;
    if (!_IO_NOT_READY_ERROR)
      return -1;
    /* Wait until the socket becomes writable */
    if (st_netfd_poll(fd, POLLOUT, timeout) < 0)
      return -1;
  }

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

APIEXPORT int st_poll(struct pollfd *pds, int npds, st_utime_t timeout)
{
 return(_st_poll(pds,npds,timeout,TRUE));
}
