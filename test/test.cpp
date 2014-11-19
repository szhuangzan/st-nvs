#include "st.h"
#include "neterr.h"
#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <errno.h>
#include <string>
#include <iostream>

static st_thread_t maintid;
static int count=0;

#pragma comment(lib, "st.lib")
#pragma comment(lib, "WS2_32.lib")
  std::string ss="ss";
  class  test
  {
  public:
	  test()
	  {
		  std::cout<<"ctor..."<<std::endl;
	  }
	  ~test()
	  {
 std::cout<<"dtor..."<<std::endl;
	  }
  };

  void throw_test()
  {
	  try
	  {
		   throw "ss";
	  }
	  catch(...)
	  {
	  }
		
  }

  void i_mythread(void*arg)
  {
	  st_thread_t t1=(st_thread_t)arg;
	
	   

	  st_sleep(10);
	  st_thread_interrupt(maintid);
	  printf("thread %d started. main=%d\n",st_thread_self(),t1);
	

	  try
	  {
		  throw_test();
	  }
	  catch (...)
	  {
		  throw "ss";
	  }

  }

  void test(void* arg)
  {
	 
	  try
	  {
		  i_mythread(arg);
	  }
	  catch(...)
	  {
		  printf("sss\n");
	  }
  }

  void print()
  {
	  std::string ss = "ss";
	  std::cout<<ss<<std::endl;
  }


void *mythread(void *arg)
 {


test(arg);

count++;
  
  return(0);
 }
#include <windows.h>
extern int server(int argc, char *argv[]);
int main(int argc, char *argv[])
 {
	
 st_thread_t tid;
int rc=0;
 //server(argc, argv);
 //
 //return 0;
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

