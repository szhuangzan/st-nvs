#include <stdio.h>
#include <Windows.h>


// windows�������ο�����:http://blog.csdn.net/zhangpeng_linux/article/details/7001084

#include "st.h"
#include "SMUSync.h"
#define SLEEP_TIME 5000 //���ʱ��

#define FILE_PATH "C:\\log.txt" //��Ϣ����ļ�

bool brun=false;

SERVICE_STATUS servicestatus;

SERVICE_STATUS_HANDLE hstatus;


int WriteToLog(char* str);

void WINAPI ServiceMain(int argc, char** argv);

void WINAPI CtrlHandler(DWORD request);

int InitService();


#pragma comment(lib, "st.lib")
#pragma comment(lib, "WS2_32.lib")


int WriteToLog(char* str)

{

	FILE* pfile;

	fopen_s(&pfile,FILE_PATH,"a+");

	if (pfile==NULL)

	{

		return -1;

	}

	fprintf_s(pfile,"%s\n",str);

	fclose(pfile);

	return 0;

}



void WINAPI ServiceMain(int argc, char** argv)

{

	servicestatus.dwServiceType = SERVICE_WIN32;

	servicestatus.dwCurrentState = SERVICE_START_PENDING;

	servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;//�ڱ�����ֻ����ϵͳ�ػ���ֹͣ�������ֿ�������

	servicestatus.dwWin32ExitCode = 0;

	servicestatus.dwServiceSpecificExitCode = 0;

	servicestatus.dwCheckPoint = 0;

	servicestatus.dwWaitHint = 0;

	hstatus = ::RegisterServiceCtrlHandler("testservice", CtrlHandler);

	if (hstatus==0)

	{

		WriteToLog("RegisterServiceCtrlHandler failed");

		return;

	}

	WriteToLog("RegisterServiceCtrlHandler success");

	//��SCM ��������״̬

	servicestatus.dwCurrentState = SERVICE_RUNNING;

	SetServiceStatus (hstatus, &servicestatus);

	//����Ϳ�ʼ����ѭ���ˣ������������Լ�ϣ���������Ĺ���

	brun=true;

	MEMORYSTATUS memstatus;

	char str[100];

	memset(str,'\0',100);

	while (brun)

	{

		GlobalMemoryStatus(&memstatus);

		int availmb=memstatus.dwAvailPhys/1024/1024;

		sprintf_s(str,100,"available memory is %dMB",availmb);

		WriteToLog(str);

		Sleep(SLEEP_TIME);

	}

	WriteToLog("service stopped");

}

void WINAPI CtrlHandler(DWORD request)

{

	switch (request)

	{

	case SERVICE_CONTROL_STOP:

		brun=false;

		servicestatus.dwCurrentState = SERVICE_STOPPED;

		break;

	case SERVICE_CONTROL_SHUTDOWN:

		brun=false;

		servicestatus.dwCurrentState = SERVICE_STOPPED;

		break;

	default:

		break;

	}

	SetServiceStatus (hstatus, &servicestatus);



}

void* handle_connect(void*arg)
{

	printf("ok\n");
	SMUSync sync;
	sync.SetPlatAddr("www.home.see1000.com", 80);
	sync.SetPlatAuthInfo("","");
	sync.SetPlatInfo("","","");
	sync.SetPlatAuthInfo("18280100206", "123456q");
	sync.Login();

	return NULL;
}


void main()
{

	st_init();
	st_thread_create(handle_connect, 0,0,0);
	st_sleep(10000);
	Sleep(10000);

	SERVICE_TABLE_ENTRY entrytable[2];

	entrytable[0].lpServiceName="testservice";

	entrytable[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)ServiceMain;

	entrytable[1].lpServiceName=NULL;

	entrytable[1].lpServiceProc=NULL;

	StartServiceCtrlDispatcher(entrytable);

}