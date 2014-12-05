#include "st.h"
#include "neterr.h"
#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <errno.h>
#include "server.hpp"
static st_thread_t maintid;
static int count=0;

#pragma comment(lib, "st.lib")
#pragma comment(lib, "WS2_32.lib")

#include <windows.h>
extern int server(int argc, char *argv[]);
void* handle_connect(void*arg)
{
	char buf[1024] = "Provider=SQLOLEDB;Server=182.131.21.104;Database=alarm;User ID=alarm_user;Password=Huamaitel.com0822;Data Source=182.131.21.104,3199";
	st_db_connect(buf);
	printf("ok\n");
	return NULL;
}
int main(int argc, char *argv[])
{

	st_init();
	st_thread_create(handle_connect, 0,0,0);
	st_sleep(10000);
	/*st_netfd_t srvfd = NULL;
	int n = 0;
	struct sockaddr_in serv_addr;
	struct hostent *hp = 0;
	unsigned short port = 0;



	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(0);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	srvfd = st_netfd_listen(serv_addr);
	st_thread_create(handle_connect, 0,0,0);
	st_sleep(10000);*/
	ServerHandler srv;

	srv.run();

	return(0);
}

