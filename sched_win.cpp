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
extern HANDLE  _st_comletion_port;
#endif

extern st_queue_t*	_st_lock_free_queue;
extern st_fifo_t*  _st_fifo;

HANDLE	_st_notify_event;
HANDLE  _st_semaphore;

int _st_wait(st_utime_t timeout)
{
	st_thread_t *me = _ST_CURRENT_THREAD();

	if (me->flags & _ST_FL_INTERRUPT) {
		me->flags &= ~_ST_FL_INTERRUPT;
		errno = EINTR;
		return 1;
	}

	/*  if (timeout != ST_UTIME_NO_TIMEOUT)
		_ST_ADD_SLEEPQ(me, timeout);*/
	me->state = _ST_ST_IO_WAIT;

	_ST_SWITCH_CONTEXT(me);

	if (me->flags & _ST_FL_INTERRUPT) {
		me->flags &= ~_ST_FL_INTERRUPT;
		errno = EINTR;
		return 1;
	}
	return 0;
}

void _st_vp_schedule(void)
{
	st_thread_t *thread;

	while (_ST_RUNQ.next != &_ST_RUNQ) {
		/* Pull thread off of the run queue */
		thread = _ST_THREAD_PTR(_ST_RUNQ.next);
		_ST_DEL_RUNQ(thread);
		ST_ASSERT(thread->state == _ST_ST_RUNNABLE);
		/* Resume the thread */
		thread->state = _ST_ST_RUNNING;
		_ST_RESTORE_CONTEXT(thread);
	}

   {
		/* If there are no threads to run, switch to the idle thread */
		thread = _st_this_vp.idle_thread;
		ST_ASSERT(thread->state == _ST_ST_RUNNABLE);
		thread->state = _ST_ST_RUNNING;
		_ST_RESTORE_CONTEXT(thread);
		/* Resume the thread */
	
	}


}


/*
 * Initialize this Virtual Processor
 */
extern "C" int st_init(void)
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
	thread = (st_thread_t *)calloc(1, sizeof(st_thread_t)+
		(ST_KEYS_MAX * sizeof(void *)));
	if (!thread)
		return -1;
	thread->private_data = (void **)(thread + 1);
	thread->state = _ST_ST_RUNNING;
	thread->flags = _ST_FL_PRIMORDIAL;
	_ST_SET_CURRENT_THREAD(thread);
	_st_active_count++;

	_st_notify_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	_st_semaphore		 = CreateSemaphore(NULL, 0, 10,NULL);
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
	struct timeval timeout, *tvp = 0;
	st_utime_t min_timeout;
	st_thread_t* thread=0;
	DWORD	ret = 0;

	if (ST_CLIST_IS_EMPTY(&_ST_SLEEPQ)) {
		tvp = NULL;
	}
	else {
		min_timeout = (_ST_THREAD_PTR(_ST_SLEEPQ.next))->sleep;
		timeout.tv_sec = (int)(min_timeout / 1000000);
		timeout.tv_usec = (int)(min_timeout % 1000000);
		tvp = &timeout;
	}

	ret = WaitForSingleObject(_st_notify_event, 1000);

	if(ret == WAIT_OBJECT_0)
	{
		while(_st_lock_free_dequeue(_st_lock_free_queue, &thread))
		{
			if (thread->flags & _ST_FL_ON_SLEEPQ)
				_ST_DEL_SLEEPQ(thread, 0);
			thread->state = _ST_ST_RUNNABLE;
			_ST_ADD_RUNQ(thread);
		}
		/*else
		{
			printf("fail....\n");
		}*/

		//while(thread = _st_lock_free_dequeue(_st_lock_free_queue))
		//{
		//	if (thread)
		//	{
		//		if (thread->flags & _ST_FL_ON_SLEEPQ)
		//			_ST_DEL_SLEEPQ(thread, 0);
		//		thread->state = _ST_ST_RUNNABLE;

		//		_ST_ADD_RUNQ(thread);
		//	}
		//}
	/*	st_thread_cell_t* cell = st_stack_pop(_st_fifo);
		{
			if (cell && cell->thread)
			{
				if (cell->thread->flags & _ST_FL_ON_SLEEPQ)
					_ST_DEL_SLEEPQ(thread, 0);
				cell->thread->state = _ST_ST_RUNNABLE;
				
				_ST_ADD_RUNQ(cell->thread);
			}
		}*/
		//	http://wenku.baidu.com/view/44ff811455270722192ef7fb.html
		
		
	}
	ResetEvent(_st_notify_event);
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
	}
	else {
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
		}
		else {
			t = _ST_THREAD_PTR(q);
			t->sleep += thread->sleep;
		}
	}
	else {
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

	thread->flags |= _ST_FL_INTERRUPT;

	if (thread->state == _ST_ST_RUNNING || thread->state == _ST_ST_RUNNABLE)
		return;

	if (thread->flags & _ST_FL_ON_SLEEPQ)
		_ST_DEL_SLEEPQ(thread, 0);

	/* Make thread runnable */
	thread->state = _ST_ST_RUNNABLE;
	_ST_ADD_RUNQ(thread);
}


extern "C" st_thread_t *st_thread_create(void *(*start)(void *arg), void *arg,
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
	thread = (st_thread_t *)sp;
	sp = sp + sizeof(st_thread_t);
	ptds = (void **)sp;
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

	if (!thread->context_switch)
	{
		thread->context_switch = (st_context_switch_t*)calloc(1, sizeof(st_context_switch_t));
		thread->context_switch->thread = thread;
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




// iocp thread poll

APIEXPORT DWORD WINAPI _st_iocp_thread_pool(LPVOID param)
{
	DWORD bytes;
	st_per_handle_data*  per_handler_data = NULL;			 //单句柄数据
	st_context_switch_t* context = NULL;

	int  ret = 0;
	int	 err  = 0;

	while(1)
	{
		bytes = -1;
		ret = GetQueuedCompletionStatus(
			_st_comletion_port,                 //原先的完成端口句柄
			&bytes,							 //重叠操作完成的字节数
			(LPDWORD)&per_handler_data,			//原先和完成端口句柄关联起来的单句柄数据
			(LPOVERLAPPED*)&context,		 //用于接收已完成的IO操作的重叠结构
			INFINITE);
		context->valid_len = -1;
		err = WSAGetLastError();
		if (!context){printf("exit...\n"); break;}
		if ((!ret || bytes == 0) && context->operator_type != ACCEPT_OPER && context->operator_type != CONNECT_OPER)
		{
			printf("%s : %d\n", __FUNCTION__, __LINE__);
			closesocket(per_handler_data->socket);
			// GlobalFree(per_handler_data);
			errno = _st_GetError(0);
		}

		if (context)
		{
			context->valid_len = bytes;

			//这是AcceptEx函数处理完成，在下面处理
			if (context->operator_type == ACCEPT_OPER)     //处理连接操作
			{
				per_handler_data->socket = context->osfd;
				CreateIoCompletionPort((HANDLE)per_handler_data->socket,
					_st_comletion_port, (ULONG_PTR)per_handler_data, 0);
			}
			//st_thread_cell_t*cell = (st_thread_cell_t*)calloc(1,sizeof(st_thread_cell_t));
		//	cell->thread = context->thread;
			//st_stack_push(_st_fifo, cell);
			//_st_lock_free_enqueue(_st_lock_free_queue, context->thread);
			//ReleaseSemaphore(_st_semaphore,1,&count);
			_st_lock_free_enqueue(_st_lock_free_queue, context->thread);
			SetEvent(_st_notify_event);
		}
		
	}
	return 0;
}
