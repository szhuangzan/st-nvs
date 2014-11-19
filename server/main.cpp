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
int main(int argc, char *argv[])
{
	ServerHandler srv;
	srv.run();

	return(0);
}

