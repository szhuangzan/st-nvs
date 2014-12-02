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

#ifndef __ST_COMMON_H__
#define __ST_COMMON_H__

#include <stddef.h>
#include <sys/types.h>

#include<errno.h>
#include <WinSock2.h>

#include <MSWSock.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
#define APIEXPORT
#else

#define APIEXPORT
#endif
#include <setjmp.h>

/* Enable assertions only if DEBUG is defined */
#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>
#define ST_ASSERT(expr) assert(expr)

#define ST_BEGIN_MACRO  {
#define ST_END_MACRO    }

#include "md.h"

#define ACCEPT_OPER 0
#define SEND_OPER	1
#define RECV_OPER   2


/*****************************************
 * Circular linked list definitions
 */

typedef struct _st_clist {
  struct _st_clist *next;
  struct _st_clist *prev;
} st_clist_t;

/* Insert element "_e" into the list, before "_l" */
#define ST_INSERT_BEFORE(_e,_l)  \
    ST_BEGIN_MACRO               \
        (_e)->next = (_l);       \
        (_e)->prev = (_l)->prev; \
        (_l)->prev->next = (_e); \
        (_l)->prev = (_e);       \
    ST_END_MACRO

/* Insert element "_e" into the list, after "_l" */
#define ST_INSERT_AFTER(_e,_l)   \
    ST_BEGIN_MACRO               \
        (_e)->next = (_l)->next; \
        (_e)->prev = (_l);       \
        (_l)->next->prev = (_e); \
        (_l)->next = (_e);       \
    ST_END_MACRO

/* Return the element following element "_e" */
#define ST_NEXT_LINK(_e)  ((_e)->next)

/* Append an element "_e" to the end of the list "_l" */
#define ST_APPEND_LINK(_e,_l) ST_INSERT_BEFORE(_e,_l)

/* Insert an element "_e" at the head of the list "_l" */
#define ST_INSERT_LINK(_e,_l) ST_INSERT_AFTER(_e,_l)

/* Return the head/tail of the list */
#define ST_LIST_HEAD(_l) (_l)->next
#define ST_LIST_TAIL(_l) (_l)->prev

/* Remove the element "_e" from it's circular list */
#define ST_REMOVE_LINK(_e)             \
    ST_BEGIN_MACRO                     \
        (_e)->prev->next = (_e)->next; \
        (_e)->next->prev = (_e)->prev; \
    ST_END_MACRO

/* Return non-zero if the given circular list "_l" is empty, */
/* zero if the circular list is not empty */
#define ST_CLIST_IS_EMPTY(_l) \
    ((_l)->next == (_l))

/* Initialize a circular list */
#define ST_INIT_CLIST(_l)  \
    ST_BEGIN_MACRO         \
        (_l)->next = (_l); \
        (_l)->prev = (_l); \
    ST_END_MACRO

#define ST_INIT_STATIC_CLIST(_l) \
    {(_l), (_l)}


/*****************************************
 * Basic types definitions
 */

typedef unsigned long st_utime_t;
typedef void  (*st_destructor_t)(void *);


typedef struct _st_stack {
  st_clist_t links;
  char *vaddr;                /* Base of stack's allocated memory */
  int  vaddr_size;            /* Size of stack's allocated memory */
  int  stk_size;              /* Size of usable portion of the stack */
  char *stk_bottom;           /* Lowest address of stack's usable portion */
  char *stk_top;              /* Highest address of stack's usable portion */
  void *sp;                   /* Stack pointer from C's point of view */
} st_stack_t;


typedef struct _st_cond {
  st_clist_t wait_q;          /* Condition variable wait queue */
} st_cond_t;


struct _st_context_switch_t;

typedef struct _st_thread {

  int		 state;                  /* Thread's state */
  int		flags;                  /* Thread's flags */

  void *(*start)(void *arg);  /* The start function of the thread */
  void *arg;                  /* Argument of the start function */
  void *retval;               /* Return value of the start function */

  st_stack_t *stack;          /* Info about thread's stack */

  st_clist_t links;           /* For putting on run/sleep/zombie queue */
  st_clist_t wait_links;      /* For putting on mutex/condvar wait queue */
  st_utime_t sleep;           /* Sleep time when thread is sleeping */

  void **private_data;        /* Per thread private data */

  st_cond_t *term;            /* Termination condition variable for join */
  struct _st_context_switch_t* context_switch;
  jmp_buf context;            /* Thread's context */
} st_thread_t;


typedef struct _st_context_switch_t{
	OVERLAPPED		 overlapped;
	int				 operator_type;
	int				 valid_len;
	SOCKET			 osfd;
	WSABUF			 buffer;
	st_thread_t*      thread;
}st_context_switch_t;


typedef struct _st_mutex {
  st_thread_t *owner;         /* Current mutex owner */
  st_clist_t  wait_q;         /* Mutex wait queue */
} st_mutex_t;



typedef struct _st_vp {
  st_thread_t *idle_thread;   /* Idle thread for this vp */
  st_utime_t last_clock;      /* The last time we went into vp_check_clock() */

  st_clist_t run_q;           /* run queue for this vp */
  st_clist_t io_q;            /* io queue for this vp */
  st_clist_t sleep_q;         /* sleep queue for this vp */
  st_clist_t zombie_q;        /* zombie queue for this vp */
  st_utime_t sleep_max;
  int maxfd;
  int pagesize;

  fd_set fd_read_set, fd_write_set, fd_exception_set;
  int fd_ref_cnts[FD_SETSIZE][3];
} st_vp_t;



typedef struct _st_per_handle_data
{
	SOCKET socket;					 //相关的套接字
	SOCKADDR_STORAGE clientAddr;     //客户端的地址

}st_per_handle_data;

typedef struct _st_netfd {
	
	SOCKET     osfd;
    int			inuse;                  /* In-use flag */
    void	    *private_data;         /* Per descriptor private data */
    st_destructor_t destructor; /* Private data destructor function */
    void			*aux_data;             /* Auxiliary data for internal use */
    struct _st_netfd *next;     /* For putting on the free list */
} st_netfd_t;


/*****************************************
 * Current vp and thread
 */

extern st_vp_t    _st_this_vp;
extern st_thread_t  *_st_this_thread;

#define _ST_CURRENT_THREAD()            (_st_this_thread)
#define _ST_SET_CURRENT_THREAD(_thread) (_st_this_thread = (_thread))

#define _ST_PAGE_SIZE                   (_st_this_vp.pagesize)

#define _ST_FD_READ_SET                 (_st_this_vp.fd_read_set)
#define _ST_FD_WRITE_SET                (_st_this_vp.fd_write_set)
#define _ST_FD_EXCEPTION_SET            (_st_this_vp.fd_exception_set)

#define _ST_FD_READ_CNT(fd)             (_st_this_vp.fd_ref_cnts[fd][0])
#define _ST_FD_WRITE_CNT(fd)            (_st_this_vp.fd_ref_cnts[fd][1])
#define _ST_FD_EXCEPTION_CNT(fd)        (_st_this_vp.fd_ref_cnts[fd][2])

#define _ST_SLEEPQMAX                   (_st_this_vp.sleep_max)
#define _ST_MAX_OSFD                    (_st_this_vp.maxfd)

#define _ST_IOQ                         (_st_this_vp.io_q)
#define _ST_RUNQ                        (_st_this_vp.run_q)
#define _ST_SLEEPQ                      (_st_this_vp.sleep_q)
#define _ST_ZOMBIEQ                     (_st_this_vp.zombie_q)


/*****************************************
 * vp queues operations
 */

#define _ST_ADD_IOQ(_pq)    ST_APPEND_LINK(&_pq.links, &_ST_IOQ)
#define _ST_DEL_IOQ(_pq)    ST_REMOVE_LINK(&_pq.links)

#define _ST_ADD_RUNQ(_thr)  ST_APPEND_LINK(&(_thr)->links, &_ST_RUNQ)
#define _ST_DEL_RUNQ(_thr)  ST_REMOVE_LINK(&(_thr)->links)

#define _ST_ADD_SLEEPQ(_thr, _timeout)  _st_add_sleep_q(_thr, _timeout)
#define _ST_DEL_SLEEPQ(_thr, _expired)  _st_del_sleep_q(_thr, _expired)

#define _ST_ADD_ZOMBIEQ(_thr)  ST_APPEND_LINK(&(_thr)->links, &_ST_ZOMBIEQ)
#define _ST_DEL_ZOMBIEQ(_thr)  ST_REMOVE_LINK(&(_thr)->links)


/*****************************************
 * Thread states and flags
 */

#define _ST_ST_RUNNING      0
#define _ST_ST_RUNNABLE     1
#define _ST_ST_IO_WAIT      2
#define _ST_ST_LOCK_WAIT    3
#define _ST_ST_COND_WAIT    4
#define _ST_ST_SLEEPING     5
#define _ST_ST_ZOMBIE       6

#define _ST_FL_PRIMORDIAL   0x01
#define _ST_FL_IDLE_THREAD  0x02
#define _ST_FL_ON_SLEEPQ    0x04
#define _ST_FL_INTERRUPT    0x08
#define _ST_FL_TIMEDOUT     0x10


/*****************************************
 * Pointer conversion
 */

#ifndef offsetof
#define offsetof(type, identifier) ((size_t)&(((type *)0)->identifier))
#endif

#define _ST_THREAD_PTR(_qp)       \
    ((st_thread_t *)((char *)(_qp) - offsetof(st_thread_t, links)))

#define _ST_THREAD_WAITQ_PTR(_qp) \
    ((st_thread_t *)((char *)(_qp) - offsetof(st_thread_t, wait_links)))

#define _ST_THREAD_STACK_PTR(_qp) \
    ((st_stack_t *)((char*)(_qp) - offsetof(st_stack_t, links)))

#define _ST_POLLQUEUE_PTR(_qp)    \
    ((st_pollq_t *)((char *)(_qp) - offsetof(st_pollq_t, thread_link)))


/*****************************************
 * Constants
 */

#define ST_UTIME_NO_TIMEOUT (-1)
#define ST_DEFAULT_STACK_SIZE (64*1024)
#define ST_KEYS_MAX 16


/*****************************************
 * Threads context switching
 */

/*
 * Switch away from the current thread context by saving its state and
 * calling the thread scheduler
 */
#define _ST_SWITCH_CONTEXT(_thread)       \
    ST_BEGIN_MACRO                        \
    if (!MD_SETJMP((_thread)->context)) { \
      _st_vp_schedule();                  \
    }                                     \
    ST_END_MACRO

/*
 * Restore a thread context that was saved by _ST_SWITCH_CONTEXT or
 * initialized by _ST_INIT_CONTEXT
 */
#define _ST_RESTORE_CONTEXT(_thread)   \
    ST_BEGIN_MACRO                     \
    _ST_SET_CURRENT_THREAD(_thread);   \
    MD_LONGJMP((_thread)->context, 1); \
    ST_END_MACRO

/*
 * Initialize the thread context preparing it to execute _main
 */
#ifdef MD_INIT_CONTEXT
#define _ST_INIT_CONTEXT MD_INIT_CONTEXT
#else
#error Unknown OS
#endif

/*
 * Number of bytes reserved under the stack "bottom"
 */
#define _ST_STACK_PAD_SIZE MD_STACK_PAD_SIZE


/*****************************************
 * Forward declarations
 */


void _st_vp_schedule(void);
void _st_vp_idle(void);
void _st_vp_check_clock(void);
void _st_find_bad_fd(void);
void *_st_idle_thread_start(void *arg);
void _st_thread_main(void);
void _st_thread_cleanup(st_thread_t *thread);
void _st_add_sleep_q(st_thread_t *thread, st_utime_t timeout);
void _st_del_sleep_q(st_thread_t *thread, int expired);
st_stack_t *_st_stack_new(int stack_size);
void _st_stack_free(st_stack_t *ts);
int _st_io_init(void);
int _st_wait(st_netfd_t* fd, st_utime_t timeout);

APIEXPORT st_utime_t st_utime(void);
APIEXPORT st_cond_t *st_cond_new(void);
APIEXPORT int st_cond_destroy(st_cond_t *cvar);
APIEXPORT int st_cond_timedwait(st_cond_t *cvar, st_utime_t timeout);
APIEXPORT int st_cond_signal(st_cond_t *cvar);
APIEXPORT ssize_t st_read(st_netfd_t *fd, void *buf, size_t nbyte, st_utime_t timeout);
APIEXPORT ssize_t st_write(st_netfd_t *fd, const void *buf, size_t nbyte,
                 st_utime_t timeout);
APIEXPORT int st_poll(struct pollfd *pds, int npds, st_utime_t timeout);
APIEXPORT st_thread_t *st_thread_create(void *(*start)(void *arg), void *arg,
                              int joinable, int stk_size);
APIEXPORT st_netfd_t* st_netfd_listen(struct sockaddr_in addr);

#endif /* !__ST_COMMON_H__ */
