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


/////////////////////////
// Array based lock free
// queue 
/////////////////////////

#include <malloc.h>

#include "common.h"

#define MAX_LOCK_FREE_SIZE 8192



CRITICAL_SECTION _cs;
//st_lock_free_queue_t* _st_lock_free_queue_open()
//{
//	st_lock_free_queue_t* queue = calloc(1, sizeof(st_lock_free_queue_t));
//	queue->length = MAX_LOCK_FREE_SIZE;
//	queue->thread = calloc(1, queue->length);
//	InitializeCriticalSection(&_cs);
//	return queue;
//}

void _st_lock_free_queue_resize(st_lock_free_queue_t* queue)
{
	// Declare temporary size variable
	int nNewSize = 0;	
	void* data = NULL;
	CRITICAL_SECTION cs;

	// double the size of our queue
	InterlockedExchangeAdd(&nNewSize,2 * queue->length);

	// allocate the new array
	data = calloc(1, sizeof(void*)*nNewSize);

	// Initialize the critical section to protect the copy
	InitializeCriticalSection(&cs);

	// Enter the critical section
	EnterCriticalSection(&cs);

	// copy the old data
	memcpy_s((void*)data, sizeof(void*)*nNewSize,(void*)queue->thread,sizeof(void*)*queue->length);		

	// dump the old array
	free(queue->thread);

	// save the new array
	queue->thread = data;

	// save the new size
	queue->length = nNewSize;

	// Leave the critical section
	LeaveCriticalSection(&cs);

	// Delete the critical section
	DeleteCriticalSection(&cs);

	return;
}


/*void _st_lock_free_enqueue(st_lock_free_queue_t* queue, st_thread_t* thread)
{
	// temporary write index and size
	volatile int nTempWrite, nTempSize;

	// atomic copy of the originals to temporary storage
	InterlockedExchange(&nTempWrite, queue->write_cnt);
	InterlockedExchange(&nTempSize, queue->length);

	// increment before bad things happen
	InterlockedIncrement(&queue->write_cnt);

	// check to make sure we haven't exceeded our storage 
	if(nTempWrite == nTempSize)
	{
		// we should resize the array even if it means using a lock
		_st_lock_free_queue_resize(queue);		
	}
	queue->thread[nTempWrite] = (void*)thread;
}

// returns false if queue is empty
st_thread_t* _st_lock_free_dequeue(st_lock_free_queue_t* queue) 
{
	st_thread_t* thread = NULL;
	// temporary write index and size
	volatile int nTempWrite, nTempRead;

	// atomic copy of the originals to temporary storage
	InterlockedExchange(&nTempWrite, queue->write_cnt);
	InterlockedExchange(&nTempRead,  queue->read_cnt);

	// increment before bad things happen
	InterlockedIncrement(&queue->read_cnt);

	// check to see if queue is empty
	if(nTempRead == nTempWrite)
	{
		// reset both indices
		InterlockedCompareExchange(&queue->read_cnt,0,nTempRead+1);
		InterlockedCompareExchange(&queue->write_cnt,0,nTempWrite);
		return NULL;
	}

	thread = queue->thread[nTempRead];
	return thread;
}
*/



//
//void _st_lock_free_enqueue(st_lock_free_queue_t* queue, int* thread)
//{
//
//	//WaitForSingleObject(_st_mutex, INFINITE);
//	if(TryEnterCriticalSection(&_cs))
//	{
//		queue->thread[queue->write_cnt++] = (void*)thread;
//		LeaveCriticalSection(&_cs);
//	}
//
//	//ReleaseMutex(_st_mutex);
//}
//
//// returns false if queue is empty
//st_thread_t* _st_lock_free_dequeue(st_lock_free_queue_t* queue)
//{
//	st_thread_t* thread = NULL;
//
//	//WaitForSingleObject(_st_mutex, INFINITE);
//	if(TryEnterCriticalSection(&_cs))
//	{
//		if (queue->read_cnt == queue->write_cnt)
//		{
//			queue->read_cnt = 0;
//			queue->write_cnt = 0;
//			LeaveCriticalSection(&_cs);
//			return NULL;
//		}
//
//		thread = (st_thread_t*)queue->thread[queue->read_cnt++];
//		LeaveCriticalSection(&_cs);
//	}
//	//EnterCriticalSection();
//	// check to see if queue is empty
//
//	//ReleaseMutex(_st_mutex);
//	return thread;
//}







 st_fifo_t *st_stack_alloc(void)
{
	st_fifo_t *fp;
	if ((fp = (st_fifo_t*)malloc(sizeof(st_fifo_t))) != NULL)
	{
		fp->top = NULL;
	}
	return fp;
}

void st_stack_push(st_fifo_t *fp, st_thread_cell_t *cl)
{

	long oldseq = fp->seq;
	LONG flag = 0;
	int ok = 0;
	PVOID addr;
	do{
		oldseq = fp->seq;
		cl->next = fp->top;
		flag = InterlockedCompareExchange((volatile long*)&fp->seq, (long)(oldseq + 1), oldseq);
		if (flag == oldseq)
		{
			ok = 1;
			addr = InterlockedCompareExchangePointer((PVOID*)&fp->top, (PVOID)cl, (PVOID)cl->next);
		}
		Sleep(0);
		printf("1 faile...\n");
	} while (!ok);
}

st_thread_cell_t *st_stack_pop(st_fifo_t *fp)
{
	st_thread_cell_t *head=NULL, *next=NULL;

	long oldseq = fp->seq;
	LONG flag = 0;
	PVOID addr;
	int ok = 0;
	do
	{
		head = fp->top;
		oldseq = fp->seq;
		if (head == NULL)
		{
			break;
		}
		next = head->next;
		flag = InterlockedCompareExchange((volatile long*)&fp->seq, (long)(oldseq + 1), oldseq);
		if (flag)
		{
			ok = 1;
			addr = InterlockedCompareExchangePointer((PVOID*)&fp->top, (PVOID)next, (PVOID)head);
		}
		Sleep(0);
		printf("2 faile...\n");
	} while (!ok);
	return head;
}





void cas_back(st_pointer_t*back, const st_pointer_t* p)
{
	InterlockedExchange(&back->count, p->count);
	InterlockedExchangePointer(&back->ptr, p->ptr);
}

void cas_back_pointer(st_pointer_t*back, const st_pointer_t* p)
{
	if(NULL == p)
		return;

	InterlockedExchange(&back->count,p->count);
	InterlockedExchangePointer((PVOID)&back->ptr,p->ptr);			
}

int cas(st_pointer_t* dest,st_pointer_t compare, st_pointer_t value)
{
	if(compare.ptr==InterlockedCompareExchangePointer((PVOID volatile*)&dest->ptr,value.ptr,compare.ptr))
	{
		InterlockedExchange(&dest->count,value.count);
		return 1;
	}

	return 0;
}

st_queue_t* _st_lock_free_queue_open()
{
	st_node_t* pNode = (st_node_t*)calloc(1,sizeof(st_node_t));
	st_queue_t* pQueue = (st_queue_t*)calloc(1, sizeof(st_queue_t));

	pQueue->head.ptr = pQueue->tail.ptr = pNode;
	return pQueue;
}

void _st_lock_free_queue_close(st_queue_t* queue)
{

}

void _st_lock_free_enqueue(st_queue_t* queue, st_thread_t* thread)
{
	// Allocate a new node from the free list
	// Keep trying until Enqueue is done
	char bEnqueueNotDone = 1;
	char nNullTail       = 1;

	st_node_t* pNode = (st_node_t*)calloc(1, sizeof(st_node_t));

	// Copy enqueued value into node
	pNode->value = thread;




	while(bEnqueueNotDone)
	{
		// Read Tail.ptr and Tail.count together
		st_pointer_t tail;
		st_pointer_t next;
		st_pointer_t tmp;
		cas_back(&tail, &queue->tail);


		nNullTail = (NULL==tail.ptr); 
		// Read next ptr and count fields together
		next.ptr = (nNullTail)? NULL : tail.ptr->next.ptr;
		next.count = (nNullTail)? 0 : tail.ptr->next.count;


		// Are tail and next consistent?
		if(tail.count == queue->tail.count && tail.ptr == queue->tail.ptr)
		{
			if(NULL == next.ptr) // Was Tail pointing to the last node?
			{
				// Try to link node at the end of the linked list
				tmp.count = next.count+1;
				tmp.ptr = pNode;
				if(cas(&tail.ptr->next, next, tmp) == 1)
				{
					bEnqueueNotDone = 0;
				} // endif

			} // endif

			else // Tail was not pointing to the last node
			{
				// Try to swing Tail to the next node
				tmp.count = tail.count+1+1;
				tmp.ptr = next.ptr;
				cas(&queue->tail, tail,tmp);
			}

		} // endif

	} // endloop
}

int _st_lock_free_dequeue(st_queue_t* queue, st_thread_t** thread)
{
	st_pointer_t head,tail,next;
	// Keep trying until Dequeue is done
	int bDequeNotDone = 1;
	st_pointer_t tmp;
	while(bDequeNotDone)
	{
		// Read Head
		head = queue->head;
		// Read Tail
		cas_back(&tail, &queue->tail);

		if(head.ptr == NULL)
		{
			// queue is empty
			return 0;
		}

		// Read Head.ptr->next
		
		cas_back_pointer(&next, &head.ptr->next);

		// Are head, tail, and next consistent
		if(head.count == queue->head.count && head.ptr == queue->head.ptr)
		{
			if(head.ptr == tail.ptr) // is tail falling behind?
			{
				// Is the Queue empty
				if(NULL == next.ptr)
				{
					// queue is empty cannot deque
					return 0;
				}
				tmp.ptr = next.ptr;
				tmp.count = tail.count+1;
				cas(&queue->tail,tail, tmp); // Tail is falling behind. Try to advance it
			} // endif

			else // no need to deal with tail
			{
				// read value before CAS otherwise another deque might try to free the next node
				*thread = next.ptr->value;

				tmp.ptr = next.ptr;
				tmp.count = head.count+1;

				// try to swing Head to the next node
				if(cas(&queue->head,head, tmp) )
				{
					bDequeNotDone = 0;
				}
			}

		} // endif

	} // endloop

	// It is now safe to free the old dummy node
	free(head.ptr);

	// queue was not empty, deque succeeded
	return 1;
}