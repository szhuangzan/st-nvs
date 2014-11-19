#include "st.h"
#include "neterr.h"
#include <stdio.h>
#include <errno.h>

static st_thread_t maintid;
static int count=0;

void *mythread(void *arg)
 {
  int sd;
  st_thread_t *t1=(st_thread_t)arg;

  printf("thread %d started. main=%d\n",st_thread_self(),t1);
  sd=socket(
  st_sleep(10);
  st_thread_interrupt(maintid);
  printf("thread %d finished\n",st_thread_self());
  count++;
  return(0);
 }

int main(int argc, char *argv[])
 {
  st_thread_t tid;
  int rc;

  st_init();
  rc=EINTR;
  maintid=st_thread_self();
  printf("main tid=%d\n",maintid);
  while(rc == EINTR)
   {
	if(count == 10) break;
    tid=st_thread_create(mythread,maintid,1,0);
    printf("tid %d created\n",tid);
    rc=st_sleep(-1);
	if(rc == -1) rc=errno;
   }
  printf("main finished. rc=%d\n",rc);
  return(0);
 }

