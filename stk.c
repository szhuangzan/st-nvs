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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include "common.h"


/* How much space to leave between the stacks, at each end */
#define REDZONE	_ST_PAGE_SIZE

st_clist_t _st_free_stacks = ST_INIT_STATIC_CLIST(&_st_free_stacks);
int _st_num_free_stacks = 0;

static char *_st_new_stk_segment(int size);

st_stack_t *_st_stack_new(int stack_size)
{
  st_clist_t *qp;
  st_stack_t *ts;

  for (qp = _st_free_stacks.next; qp != &_st_free_stacks; qp = qp->next) {
    ts = _ST_THREAD_STACK_PTR(qp);
    if (ts->stk_size >= stack_size) {
      /* Found a stack that is big enough */
      ST_REMOVE_LINK(&ts->links);
      _st_num_free_stacks--;
      ts->links.next = NULL;
      ts->links.prev = NULL;
      return ts;
    }
  }

  /* Make a new thread stack object. */
  if ((ts = (st_stack_t *)calloc(1, sizeof(st_stack_t))) == NULL)
    return NULL;
  ts->vaddr_size = stack_size + 2*REDZONE;
  ts->vaddr = _st_new_stk_segment(ts->vaddr_size);
  if (!ts->vaddr) {
    free(ts);
    return NULL;
  }
  ts->stk_size = stack_size;
  ts->stk_bottom = ts->vaddr + REDZONE;
  ts->stk_top = ts->stk_bottom + stack_size;

#ifdef DEBUG
  mprotect(ts->vaddr, REDZONE, PROT_NONE);
  mprotect(ts->stk_top, REDZONE, PROT_NONE);
#endif

  return ts;
}


/*
 * Free the stack for the current thread
 */
void _st_stack_free(st_stack_t *ts)
{
  if (!ts)
    return;

  /* Put the stack on the free list */
  ST_APPEND_LINK(&ts->links, _st_free_stacks.prev);
  _st_num_free_stacks++;
}


static char *_st_new_stk_segment(int size)
{
#ifdef MALLOC_STACK
  void *vaddr = malloc(size);
#else
  static int zero_fd = -1;
  int mmap_flags = MAP_PRIVATE;
  void *vaddr;

#if defined (MD_USE_SYSV_ANON_MMAP)
  if (zero_fd < 0) {
    if ((zero_fd = open("/dev/zero", O_RDWR, 0)) < 0)
      return NULL;
    fcntl(zero_fd, F_SETFD, FD_CLOEXEC);
  }
#elif defined (MD_USE_BSD_ANON_MMAP)
  mmap_flags |= MAP_ANON;
#else
#error Unknown OS
#endif

  vaddr = mmap(NULL, size, PROT_READ | PROT_WRITE, mmap_flags, zero_fd, 0);
  if (vaddr == (void *)MAP_FAILED)
    return NULL;

#endif /* MALLOC_STACK */

  return (char *)vaddr;
}


/* Not used */
#if 0
void _st_delete_stk_segment(char *vaddr, int size)
{
#ifdef MALLOC_STACK
  free(vaddr);
#else
  (void) munmap(vaddr, size);
#endif
}
#endif

