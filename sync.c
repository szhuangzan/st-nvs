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
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include "common.h"


extern time_t _st_curr_time;
extern st_utime_t _st_last_tset;


/*****************************************
 * Time functions
 */

APIEXPORT st_utime_t st_utime(void)
{
#ifdef MD_GET_UTIME
  MD_GET_UTIME(); 
#else
#error Unknown OS
#endif
}


APIEXPORT st_utime_t st_utime_last_clock(void)
{
  return (_st_this_vp.last_clock);
}


APIEXPORT int st_timecache_set(int on)
{
  int wason = (_st_curr_time) ? 1 : 0;

  if (on) {
    _st_curr_time = time(NULL);
    _st_last_tset = st_utime();
  } else
    _st_curr_time = 0;

  return wason;
}


APIEXPORT time_t st_time(void)
{
  if (_st_curr_time)
    return _st_curr_time;

  return time(NULL);
}


APIEXPORT int st_usleep(st_utime_t usecs)
{
  st_thread_t *me = _ST_CURRENT_THREAD();

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  me->state = _ST_ST_SLEEPING;
  _ST_ADD_SLEEPQ(me, usecs);

  _ST_SWITCH_CONTEXT(me);

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  return 0;
}


APIEXPORT int st_sleep(int secs)
{
  if(secs == -1) return(st_usleep(-1));
  else return(st_usleep(secs * 1000000));
}


/*****************************************
 * Condition variable functions
 */

APIEXPORT st_cond_t *st_cond_new(void)
{
  st_cond_t *cvar;

  cvar = (st_cond_t *) calloc(1, sizeof(st_cond_t));
  if (cvar) {
    ST_INIT_CLIST(&cvar->wait_q);
  }

  return cvar;
}


APIEXPORT int st_cond_destroy(st_cond_t *cvar)
{
  if (cvar->wait_q.next != &cvar->wait_q) {
    errno = EBUSY;
    return -1;
  }

  free(cvar);

  return 0;
}


APIEXPORT int st_cond_timedwait(st_cond_t *cvar, st_utime_t timeout)
{
  st_thread_t *me = _ST_CURRENT_THREAD();
  int rv;

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  /* Put caller thread on the condition variable's wait queue */
  me->state = _ST_ST_COND_WAIT;
  ST_APPEND_LINK(&me->wait_links, &cvar->wait_q);

  if (timeout != ST_UTIME_NO_TIMEOUT)
    _ST_ADD_SLEEPQ(me, timeout);

  _ST_SWITCH_CONTEXT(me);

  ST_REMOVE_LINK(&me->wait_links);
  rv = 0;

  if (me->flags & _ST_FL_TIMEDOUT) {
    me->flags &= ~_ST_FL_TIMEDOUT;
    errno = ETIME;
    rv = -1;
  }
  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    rv = -1;
  }

  return rv;
}


APIEXPORT int st_cond_wait(st_cond_t *cvar)
{
  return st_cond_timedwait(cvar, ST_UTIME_NO_TIMEOUT);
}


int _st_cond_signal(st_cond_t *cvar, int broadcast)
{
  st_thread_t *thread;
  st_clist_t *q;

  for (q = cvar->wait_q.next; q != &cvar->wait_q; q = q->next) {
    thread = _ST_THREAD_WAITQ_PTR(q);
    if (thread->state == _ST_ST_COND_WAIT) {
      if (thread->flags & _ST_FL_ON_SLEEPQ)
        _ST_DEL_SLEEPQ(thread, 0);

      /* Make thread runnable */
      thread->state = _ST_ST_RUNNABLE;
      _ST_ADD_RUNQ(thread);
      if (!broadcast)
        break;
    }
  }

  return 0;
}


APIEXPORT int st_cond_signal(st_cond_t *cvar)
{
  return _st_cond_signal(cvar, 0);
}


APIEXPORT int st_cond_broadcast(st_cond_t *cvar)
{
  return _st_cond_signal(cvar, 1);
}


/*****************************************
 * Mutex functions
 */

APIEXPORT st_mutex_t *st_mutex_new(void)
{
  st_mutex_t *lock;

  lock = (st_mutex_t *) calloc(1, sizeof(st_mutex_t));
  if (lock) {
    ST_INIT_CLIST(&lock->wait_q);
    lock->owner = NULL;
  }

  return lock;
}


APIEXPORT int st_mutex_destroy(st_mutex_t *lock)
{
  if (lock->owner != NULL || lock->wait_q.next != &lock->wait_q) {
    errno = EBUSY;
    return -1;
  }

  free(lock);

  return 0;
}


APIEXPORT int st_mutex_lock(st_mutex_t *lock)
{
  st_thread_t *me = _ST_CURRENT_THREAD();

  if (me->flags & _ST_FL_INTERRUPT) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  if (lock->owner == NULL) {
    /* Got the mutex */
    lock->owner = me;
    return 0;
  }

  if (lock->owner == me) {
    errno = EDEADLK;
    return -1;
  }

  /* Put caller thread on the mutex's wait queue */
  me->state = _ST_ST_LOCK_WAIT;
  ST_APPEND_LINK(&me->wait_links, &lock->wait_q);

  _ST_SWITCH_CONTEXT(me);

  ST_REMOVE_LINK(&me->wait_links);

  if ((me->flags & _ST_FL_INTERRUPT) && lock->owner != me) {
    me->flags &= ~_ST_FL_INTERRUPT;
    errno = EINTR;
    return -1;
  }

  return 0;
}


APIEXPORT int st_mutex_unlock(st_mutex_t *lock)
{
  st_thread_t *thread;
  st_clist_t *q;

  if (lock->owner != _ST_CURRENT_THREAD()) {
    errno = EPERM;
    return -1;
  }

  for (q = lock->wait_q.next; q != &lock->wait_q; q = q->next) {
    thread = _ST_THREAD_WAITQ_PTR(q);
    if (thread->state == _ST_ST_LOCK_WAIT) {
      lock->owner = thread;
      /* Make thread runnable */
      thread->state = _ST_ST_RUNNABLE;
      _ST_ADD_RUNQ(thread);
      return 0;
    }
  }

  /* No threads waiting on this mutex */
  lock->owner = NULL;

  return 0;
}


APIEXPORT int st_mutex_trylock(st_mutex_t *lock)
{
  if (lock->owner != NULL) {
    errno = EBUSY;
    return -1;
  }

  /* Got the mutex */
  lock->owner = _ST_CURRENT_THREAD();

  return 0;
}
