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
#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#endif
#include <string.h>
#include <time.h>
#include "common.h"

/* Global data */
st_vp_t _st_this_vp;            /* This VP */
st_thread_t *_st_this_thread;   /* Current thread */
int _st_active_count = 0;       /* Active thread count */

time_t _st_curr_time = 0;       /* Current time as returned by time(2) */
st_utime_t _st_last_tset;       /* Last time it was fetched */
#ifdef WIN32
/* map winsock descriptors to small integers */
extern int fds[FD_SETSIZE+5];
#endif


int _st_poll(struct pollfd *pds, int npds, st_utime_t timeout,int os)
{
  struct pollfd *pd;
  struct pollfd *epd = pds + npds;
  st_pollq_t pq;
  st_thread_t *me = _ST_CURRENT_THREAD();
  int n;

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  for (pd = pds; pd < epd; pd++) {
    if (pd->events & POLLIN) {
      if(os == TRUE) FD_SET(pd->fd, &_ST_FD_READ_SET);
      else FD_SET(fds[pd->fd], &_ST_FD_READ_SET);
      _ST_FD_READ_CNT(pd->fd)++;
    }
    if (pd->events & POLLOUT) {
      if(os == TRUE) FD_SET(pd->fd, &_ST_FD_WRITE_SET);
      else FD_SET(fds[pd->fd], &_ST_FD_WRITE_SET);
      _ST_FD_WRITE_CNT(pd->fd)++;
    }
    if (pd->events & POLLPRI) {
      if(os == TRUE) FD_SET(pd->fd, &_ST_FD_EXCEPTION_SET);
      else FD_SET(fds[pd->fd], &_ST_FD_EXCEPTION_SET);
      _ST_FD_EXCEPTION_CNT(pd->fd)++;
    }
    if (_ST_MAX_OSFD < pd->fd)
      _ST_MAX_OSFD = pd->fd;
  }

  pq.pds = pds;
  pq.npds = npds;
  pq.thread = me;
  pq.on_ioq = 1;
  _ST_ADD_IOQ(pq);
  if (timeout != ST_UTIME_NO_TIMEOUT)
    _ST_ADD_SLEEPQ(me, timeout);
  me->state = _ST_ST_IO_WAIT;

  _ST_SWITCH_CONTEXT(me);

  n = 0;
  if (pq.on_ioq) {
    /* If we timed out, the pollq might still be on the ioq. Remove it */
    _ST_DEL_IOQ(pq);
    for (pd = pds; pd < epd; pd++) {
      if (pd->events & POLLIN) {
        if (--_ST_FD_READ_CNT(pd->fd) == 0)
          if(os == TRUE) FD_CLR(pd->fd, &_ST_FD_READ_SET);
          else FD_CLR(fds[pd->fd], &_ST_FD_READ_SET);
      }
      if (pd->events & POLLOUT) {
        if (--_ST_FD_WRITE_CNT(pd->fd) == 0)
          if(os == TRUE) FD_CLR(pd->fd, &_ST_FD_WRITE_SET);
          else FD_CLR(fds[pd->fd], &_ST_FD_WRITE_SET);
      }
      if (pd->events & POLLPRI) {
        if (--_ST_FD_EXCEPTION_CNT(pd->fd) == 0)
          if(os == TRUE) FD_CLR(pd->fd, &_ST_FD_EXCEPTION_SET);
          else FD_CLR(fds[pd->fd], &_ST_FD_EXCEPTION_SET);
      }
    }
  } else {
    /* Count the number of ready descriptors */
    for (pd = pds; pd < epd; pd++) {
      if (pd->revents)
        n++;
    }
  }

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  return n;
}

void _st_vp_schedule(void)
{
  st_thread_t *thread;

  if (_ST_RUNQ.next != &_ST_RUNQ) {
    /* Pull thread off of the run queue */
    thread = _ST_THREAD_PTR(_ST_RUNQ.next);
    _ST_DEL_RUNQ(thread);
  } else {
    /* If there are no threads to run, switch to the idle thread */
    thread = _st_this_vp.idle_thread;
  }
  ST_ASSERT(thread->state == _ST_ST_RUNNABLE);

  /* Resume the thread */
  thread->state = _ST_ST_RUNNING;
  _ST_RESTORE_CONTEXT(thread);
}


/*
 * Initialize this Virtual Processor
 */
APIEXPORT int st_init(void)
{
  st_thread_t *thread;

  if (_st_active_count) {
    /* Already initialized */
    return 0;
  }

  if (_st_io_init() < 0)
    return -1;

  memset(&_st_this_vp, 0, sizeof(st_vp_t));

  ST_INIT_CLIST(&_ST_RUNQ);
  ST_INIT_CLIST(&_ST_IOQ);
  ST_INIT_CLIST(&_ST_SLEEPQ);
  ST_INIT_CLIST(&_ST_ZOMBIEQ);

  _st_this_vp.maxfd = -1;
  _st_this_vp.pagesize = getpagesize();
  _st_this_vp.last_clock = st_utime();

  /*
   * Create idle thread
   */
  _st_this_vp.idle_thread = st_thread_create(_st_idle_thread_start,
                                             NULL, 0, 0);
  if (!_st_this_vp.idle_thread)
    return -1;
  _st_this_vp.idle_thread->flags = _ST_FL_IDLE_THREAD;
  _st_active_count--;
  _ST_DEL_RUNQ(_st_this_vp.idle_thread);

  /*
   * Initialize primordial thread
   */
  thread = (st_thread_t *) calloc(1, sizeof(st_thread_t) +
                                  (ST_KEYS_MAX * sizeof(void *)));
  if (!thread)
    return -1;
  thread->private_data = (void **) (thread + 1);
  thread->state = _ST_ST_RUNNING;
  thread->flags = _ST_FL_PRIMORDIAL;
  _ST_SET_CURRENT_THREAD(thread);
  _st_active_count++;

  return 0;
}


/*
 * Start function for the idle thread
 */
/* ARGSUSED */
void *_st_idle_thread_start(void *arg)
{
  st_thread_t *me = _ST_CURRENT_THREAD();

  while (_st_active_count > 0) {
    /* Idle vp till I/O is ready or the smallest timeout expired */
    _st_vp_idle();

    /* Check sleep queue for expired threads */
    _st_vp_check_clock();

    me->state = _ST_ST_RUNNABLE;
    _ST_SWITCH_CONTEXT(me);
  }

  /* No more threads */
  exit(0);

  /* NOTREACHED */
  return NULL;
}


void _st_vp_idle(void)
{
  struct timeval timeout, *tvp;
  fd_set r, w, e;
  fd_set *rp, *wp, *ep;
  int nfd, pq_max_osfd, osfd;
  st_clist_t *q;
  st_utime_t min_timeout;
  st_pollq_t *pq;
  int notify;
  struct pollfd *pds, *epds;
  short events, revents;

  /*
   * Assignment of fd_sets
   */
  r = _ST_FD_READ_SET;
  w = _ST_FD_WRITE_SET;
  e = _ST_FD_EXCEPTION_SET;

  rp = &r;
  wp = &w;
  ep = &e;

  if (ST_CLIST_IS_EMPTY(&_ST_SLEEPQ)) {
    tvp = NULL;
  } else {
    min_timeout = (_ST_THREAD_PTR(_ST_SLEEPQ.next))->sleep;
    timeout.tv_sec  = (int) (min_timeout / 1000000);
    timeout.tv_usec = (int) (min_timeout % 1000000);
    tvp = &timeout;
  }

  /* Check for I/O operations */
  nfd = select(_ST_MAX_OSFD + 1, rp, wp, ep, tvp);
  errno=_st_GetError(0);

  /* Notify threads that are associated with the selected descriptors */
  if (nfd > 0) {
    _ST_MAX_OSFD = -1;
    for (q = _ST_IOQ.next; q != &_ST_IOQ; q = q->next) {
      pq = _ST_POLLQUEUE_PTR(q);
      notify = 0;
      epds = pq->pds + pq->npds;
      pq_max_osfd = -1;

      for (pds = pq->pds; pds < epds; pds++) {
        osfd = pds->fd;
        events = pds->events;
        revents = 0;
        ST_ASSERT(osfd >= 0 || events == 0);
        if ((events & POLLIN) && FD_ISSET(fds[osfd], rp)) {
          revents |= POLLIN;
        }
        if ((events & POLLOUT) && FD_ISSET(fds[osfd], wp)) {
          revents |= POLLOUT;
        }
        if ((events & POLLPRI) && FD_ISSET(fds[osfd], ep)) {
          revents |= POLLPRI;
        }
        pds->revents = revents;
        if (revents) {
          notify = 1;
        }
        if (osfd > pq_max_osfd) {
          pq_max_osfd = osfd;
        }
      }
      if (notify) {
        ST_REMOVE_LINK(&pq->links);
        pq->on_ioq = 0;
        /*
         * Decrement the count of descriptors for each descriptor/event
         * because this I/O request is being removed from the ioq
         */
        for (pds = pq->pds; pds < epds; pds++) {
          osfd = pds->fd;
          events = pds->events;
          if (events & POLLIN) {
            if (--_ST_FD_READ_CNT(osfd) == 0) {
              FD_CLR(fds[osfd], &_ST_FD_READ_SET);
            }
          }
          if (events & POLLOUT) {
            if (--_ST_FD_WRITE_CNT(osfd) == 0) {
              FD_CLR(fds[osfd], &_ST_FD_WRITE_SET);
            }
          }
          if (events & POLLPRI) {
            if (--_ST_FD_EXCEPTION_CNT(osfd) == 0) {
              FD_CLR(fds[osfd], &_ST_FD_EXCEPTION_SET);
            }
          }
        }

        if (pq->thread->flags & _ST_FL_ON_SLEEPQ)
          _ST_DEL_SLEEPQ(pq->thread, 0);
        pq->thread->state = _ST_ST_RUNNABLE;
        _ST_ADD_RUNQ(pq->thread);
      } else {
        if (_ST_MAX_OSFD < pq_max_osfd)
          _ST_MAX_OSFD = pq_max_osfd;
      }
    }
  } else if (nfd < 0) {
    /*
     * It can happen when a thread closes file descriptor
     * that is being used by some other thread
     */


    if( (errno == EBADF) || (errno == ENOTSOCK) )
      _st_find_bad_fd();
  }
}


void _st_find_bad_fd(void)
{
  st_clist_t *q;
  st_pollq_t *pq;
  int notify;
  struct pollfd *pds, *epds;
  int pq_max_osfd, osfd;
  short events;

  _ST_MAX_OSFD = -1;

  for (q = _ST_IOQ.next; q != &_ST_IOQ; q = q->next) {
    pq = _ST_POLLQUEUE_PTR(q);
    notify = 0;
    epds = pq->pds + pq->npds;
    pq_max_osfd = -1;

    for (pds = pq->pds; pds < epds; pds++) {
      osfd = pds->fd;
      pds->revents = 0;
      ST_ASSERT(osfd >= 0 || pds->events == 0);
      if (pds->events == 0)
        continue;
      if (osfd > pq_max_osfd) {
        pq_max_osfd = osfd;
      }
    }

    if (notify) {
      ST_REMOVE_LINK(&pq->links);
      pq->on_ioq = 0;
      /*
       * Decrement the count of descriptors for each descriptor/event
       * because this I/O request is being removed from the ioq
       */
      for (pds = pq->pds; pds < epds; pds++) {
        osfd = pds->fd;
        events = pds->events;
        if (events & POLLIN) {
          if (--_ST_FD_READ_CNT(osfd) == 0) {
            FD_CLR(fds[osfd], &_ST_FD_READ_SET);
          }
        }
        if (events & POLLOUT) {
          if (--_ST_FD_WRITE_CNT(osfd) == 0) {
            FD_CLR(fds[osfd], &_ST_FD_WRITE_SET);
          }
        }
        if (events & POLLPRI) {
          if (--_ST_FD_EXCEPTION_CNT(osfd) == 0) {
            FD_CLR(fds[osfd], &_ST_FD_EXCEPTION_SET);
          }
        }
      }

      if (pq->thread->flags & _ST_FL_ON_SLEEPQ)
        _ST_DEL_SLEEPQ(pq->thread, 0);
      pq->thread->state = _ST_ST_RUNNABLE;
      _ST_ADD_RUNQ(pq->thread);
    } else {
      if (_ST_MAX_OSFD < pq_max_osfd)
        _ST_MAX_OSFD = pq_max_osfd;
    }
  }
}


APIEXPORT void st_thread_exit(void *retval)
{
  st_thread_t *thread = _ST_CURRENT_THREAD();

  thread->retval = retval;
  _st_thread_cleanup(thread);
  _st_active_count--;
  if (thread->term) {
    /* Put thread on the zombie queue */
    thread->state = _ST_ST_ZOMBIE;
    _ST_ADD_ZOMBIEQ(thread);

    /* Notify on our termination condition variable */
    st_cond_signal(thread->term);

    /* Switch context and come back later */
    _ST_SWITCH_CONTEXT(thread);

    /* Continue the cleanup */
    st_cond_destroy(thread->term);
    thread->term = NULL;
  }

  if (!(thread->flags & _ST_FL_PRIMORDIAL))
    _st_stack_free(thread->stack);

  /* Find another thread to run */
  _ST_SWITCH_CONTEXT(thread);
  /* Not going to land here */
}


APIEXPORT int st_thread_join(st_thread_t *thread, void **retvalp)
{
  st_cond_t *term = thread->term;

  /* Can't join a non-joinable thread */
  if (term == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (_ST_CURRENT_THREAD() == thread) {
    errno = EDEADLK;
    return -1;
  }

  /* Multiple threads can't wait on the same joinable thread */
  if (term->wait_q.next != &term->wait_q) {
    errno = EINVAL;
    return -1;
  }

  while (thread->state != _ST_ST_ZOMBIE) {
    if (st_cond_timedwait(term, ST_UTIME_NO_TIMEOUT) != 0)
      return -1;
  }

  if (retvalp)
    *retvalp = thread->retval;

  /*
   * Remove target thread from the zombie queue and make it runnable.
   * When it gets scheduled later, it will do the clean up.
   */
  thread->state = _ST_ST_RUNNABLE;
  _ST_DEL_ZOMBIEQ(thread);
  _ST_ADD_RUNQ(thread);

  return 0;
}


void _st_thread_main(void)
{
  st_thread_t *thread = _ST_CURRENT_THREAD();

  /* Run thread main */
  thread->retval = (*thread->start)(thread->arg);

  /* All done, time to go away */
  st_thread_exit(thread->retval);
}


void _st_add_sleep_q(st_thread_t *thread, st_utime_t timeout)
{
  st_utime_t sleep;
  st_clist_t *q;
  st_thread_t *t;

  /* sort onto global sleep queue */
  sleep = timeout;

  /* Check if we are longest timeout */
  if (timeout >= _ST_SLEEPQMAX) {
    ST_APPEND_LINK(&thread->links, &_ST_SLEEPQ);
    thread->sleep = timeout - _ST_SLEEPQMAX;
    _ST_SLEEPQMAX = timeout;
  } else {
    /* Sort thread into global sleep queue at appropriate point */
    q = _ST_SLEEPQ.next;

    /* Now scan the list for where to insert this entry */
    while (q != &_ST_SLEEPQ) {
      t = _ST_THREAD_PTR(q);
      if (sleep < t->sleep) {
        /* Found sleeper to insert in front of */
        break;
      }
      sleep -= t->sleep;
      q = q->next;
    }
    thread->sleep = sleep;
    ST_INSERT_BEFORE(&thread->links, q);

    /* Subtract our sleep time from the sleeper that follows us */
    ST_ASSERT(thread->links.next != &_ST_SLEEPQ);
    t = _ST_THREAD_PTR(thread->links.next);
    ST_ASSERT(_ST_THREAD_PTR(t->links.prev) == thread);
    t->sleep -= sleep;
  }

  thread->flags |= _ST_FL_ON_SLEEPQ;
}


/*
 * If "expired" is true, a sleeper has timed out
 */
void _st_del_sleep_q(st_thread_t *thread, int expired)
{
  st_clist_t *q;
  st_thread_t *t;

  /* Remove from sleep queue */
  ST_ASSERT(thread->flags & _ST_FL_ON_SLEEPQ);
  q = thread->links.next;
  if (q != &_ST_SLEEPQ) {
    if (expired) {
      _ST_SLEEPQMAX -= thread->sleep;
    } else {
      t = _ST_THREAD_PTR(q);
      t->sleep += thread->sleep;
    }
  } else {
    /*
     * Check if prev is the beginning of the list; if so,
     * we are the only element on the list.
     */
    if (thread->links.prev != &_ST_SLEEPQ)
      _ST_SLEEPQMAX -= thread->sleep;
    else
      _ST_SLEEPQMAX = 0;
  }
  thread->flags &= ~_ST_FL_ON_SLEEPQ;
  ST_REMOVE_LINK(&thread->links);
}


void _st_vp_check_clock(void)
{
  st_thread_t *thread;
  st_utime_t elapsed, now;

  now = st_utime();
  elapsed = now - _st_this_vp.last_clock;
  _st_this_vp.last_clock = now;

  if (_st_curr_time && now - _st_last_tset > 999000) {
    _st_curr_time = time(NULL);
    _st_last_tset = now;
  }

  while (_ST_SLEEPQ.next != &_ST_SLEEPQ) {
    thread = _ST_THREAD_PTR(_ST_SLEEPQ.next);
    ST_ASSERT(thread->flags & _ST_FL_ON_SLEEPQ);

    if (elapsed < thread->sleep) {
      thread->sleep -= elapsed;
      _ST_SLEEPQMAX -= elapsed;
      break;
    }

    _ST_DEL_SLEEPQ(thread, 1);
    elapsed -= thread->sleep;

    /* If thread is waiting on condition variable, set the time out flag */
    if (thread->state == _ST_ST_COND_WAIT)
      thread->flags |= _ST_FL_TIMEDOUT;

    /* Make thread runnable */
    ST_ASSERT(!(thread->flags & _ST_FL_IDLE_THREAD));
    thread->state = _ST_ST_RUNNABLE;
    _ST_ADD_RUNQ(thread);
  }
}


APIEXPORT void st_thread_interrupt(st_thread_t *thread)
{
  /* If thread is already dead */
  if (thread->state == _ST_ST_ZOMBIE)
    return;

printf("interrupting thread=%8.8p\n",thread);
  thread->flags |= _ST_FL_INTERRUPT;

  if (thread->state == _ST_ST_RUNNING || thread->state == _ST_ST_RUNNABLE)
    return;

  if (thread->flags & _ST_FL_ON_SLEEPQ)
    _ST_DEL_SLEEPQ(thread, 0);

  /* Make thread runnable */
  thread->state = _ST_ST_RUNNABLE;
  _ST_ADD_RUNQ(thread);
}


APIEXPORT st_thread_t *st_thread_create(void *(*start)(void *arg), void *arg,
                              int joinable, int stk_size)
{
  st_thread_t *thread;
  st_stack_t *stack;
  void **ptds;
  char *sp;

  /* Adjust stack size */
  if (stk_size == 0)
    stk_size = ST_DEFAULT_STACK_SIZE;
  stk_size = ((stk_size + _ST_PAGE_SIZE - 1) / _ST_PAGE_SIZE) * _ST_PAGE_SIZE;
  stack = _st_stack_new(stk_size);
  if (!stack)
    return NULL;

  /* Allocate thread object and per-thread data off the stack */
#if defined (MD_STACK_GROWS_DOWN)
  sp = stack->stk_top;
  sp = sp - (ST_KEYS_MAX * sizeof(void *));
  ptds = (void **) sp;
  sp = sp - sizeof(st_thread_t);
  thread = (st_thread_t *) sp;

  /* Make stack 64-byte aligned */
  if ((unsigned long)sp & 0x3f)
    sp = sp - ((unsigned long)sp & 0x3f);
  stack->sp = sp - _ST_STACK_PAD_SIZE;
#elif defined (MD_STACK_GROWS_UP)
  sp = stack->stk_bottom;
  thread = (st_thread_t *) sp;
  sp = sp + sizeof(st_thread_t);
  ptds = (void **) sp;
  sp = sp + (ST_KEYS_MAX * sizeof(void *));

  /* Make stack 64-byte aligned */
  if ((unsigned long)sp & 0x3f)
    sp = sp + (0x40 - ((unsigned long)sp & 0x3f));
  stack->sp = sp + _ST_STACK_PAD_SIZE;
#else
#error Unknown OS
#endif

  memset(thread, 0, sizeof(st_thread_t));
  memset(ptds, 0, ST_KEYS_MAX * sizeof(void *));

  /* Initialize thread */
  thread->private_data = ptds;
  thread->stack = stack;
  thread->start = start;
  thread->arg = arg;

  _ST_INIT_CONTEXT(thread, stack->sp, _st_thread_main);

  /* If thread is joinable, allocate a termination condition variable */
  if (joinable) {
    thread->term = st_cond_new();
    if (thread->term == NULL) {
      _st_stack_free(thread->stack);
      return NULL;
    }
  }

  /* Make thread runnable */
  thread->state = _ST_ST_RUNNABLE;
  _st_active_count++;
  _ST_ADD_RUNQ(thread);

  return thread;
}


APIEXPORT st_thread_t *st_thread_self(void)
{
  return _ST_CURRENT_THREAD();
}

