#include <stdio.h>
#include <Windows.h>
#include <TlHelp32.h>
#include<string>
#include <conio.h>

// windows服务器参考文章:http://blog.csdn.net/zhangpeng_linux/article/details/7001084

#include "st.h"
#include "SMUSync.h"
#include "SyncServer.h"
#include "ParseCfg.h"

#define HM_SYNC_DEBUG 0

#pragma comment(lib, "st.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma  comment(lib, "libeay32.lib")

#define SLEEP_TIME 5000 //间隔时间

#define LOG_MAX_SIZE 128

#define SERVER_NAME "HMSyncPlatformServer"
bool brun=false;



SERVICE_STATUS servicestatus;

SERVICE_STATUS_HANDLE hstatus;


void WINAPI ServiceMain(int argc, char** argv);

void WINAPI CtrlHandler(DWORD request);

bool    STOP_SERVER = false;
int		InitService();


class LogInfo
{
public:
	LogInfo()
		:_log_file()
	{
		char path[256] = {};
		GetModuleFileName(NULL, path, 256);
		_log_filename =path;
		int pos = _log_filename.find_last_of("\\");
		_log_filename = _log_filename.substr(0,pos+1);
		_log_filename.append("sync.log");
		_log_file = fopen(_log_filename.c_str(), "a+");
		fclose(_log_file);
		_log_file = NULL;
	}
	~LogInfo()
	{
		if(_log_file) fclose(_log_file);
		_log_file = 0;
	}
public:
	void Write(const char* buf)
	{
		std::string log;
		{
			char time[64] ={};
			timestamp(time);
			log.insert(0,time);
		}
		log.append(buf);
		log.append("\n");
#if HM_SYNC_DEBUG
		printf("%s\n", log.c_str());
#else
		i_Write(log.c_str());
#endif
		return;
	}
private:
	void timestamp(char* time)
	{
		char cur_time[64] = {};
		time_t cur = st_time();
		struct tm* cur_tm  = localtime(&cur);
		sprintf_s(cur_time, "%04d-%02d-%02d %02d:%02d:%02d ->",
			cur_tm->tm_year+1900, 
			cur_tm->tm_mon+1,
			cur_tm->tm_mday,
			cur_tm->tm_hour,
			cur_tm->tm_min,
			cur_tm->tm_sec);
		sprintf_s(time, 64,"%s ", cur_time);
	}
	void i_Write(const char* log)
	{

		if (!_log_file)
		{
			_log_file = fopen(_log_filename.c_str(), "a+");
		}


		{
			if(fwrite(log, 1, strlen(log), _log_file) == 0)
			{
				fclose(_log_file);
				_log_file = fopen(_log_filename.c_str(), "a+");

			}
			
			fflush(_log_file);

		}

		{
			_file_size = ftell(_log_file);

			if(_file_size >= LOG_MAX_SIZE*1024)
			{
				int pos = fseek(_log_file, 0, SEEK_SET);
				printf("pos = %d\n", pos);

			}
		}


	}
private:
	FILE*			_log_file;
	int				_file_size;
	std::string		_log_filename;
};

static LogInfo*			_LogInfoInst;
int WriteToLog(char* fmt, ...)
{
	if(!_LogInfoInst) _LogInfoInst = new LogInfo;
	{
		char buf[2048] = {};
		va_list arg_list;
		va_start(arg_list, fmt);
		vsnprintf(buf, 2048, fmt, arg_list);
		va_end(arg_list);
		_LogInfoInst->Write(buf);
	}
	
	return 0;
}

bool CheckNotServer()
{
	bool flag = false;
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scm != NULL){
		SC_HANDLE svc = OpenService(scm, SERVER_NAME,
			SERVICE_ALL_ACCESS);
		if (svc == NULL)
		{
			WriteToLog("Create Server...");
			flag = true;	
		}
		//以便立即从数据库中移走此条目。 
		CloseServiceHandle(scm);
	}
	return flag;
}

void   CreateServer(char* server_path)
{
	if(!CheckNotServer()) return;
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (scm != NULL){
		SC_HANDLE svc = CreateService(scm,
			SERVER_NAME, SERVER_NAME,//Service名字 
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START, //以自动方式开始 
			SERVICE_ERROR_IGNORE,
			server_path,   //Service本体程序路径， 
			NULL, NULL, NULL, NULL, NULL);
		if (svc != NULL)
		{
			CloseServiceHandle(svc);
		}
		CloseServiceHandle(scm);
	}
}




void  DeleteServer()

{
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scm != NULL){
		SC_HANDLE svc = OpenService(scm, SERVER_NAME,
			SERVICE_ALL_ACCESS);
		if (svc != NULL){
			SERVICE_STATUS ServiceStatus;
			QueryServiceStatus(svc, &ServiceStatus);
			if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)//删除前，先停止此Service. 
				ControlService(svc, SERVICE_CONTROL_STOP, &ServiceStatus);
			DeleteService(svc);
			CloseServiceHandle(svc); //删除Service后，最好再调用CloseServiceHandle 

		}
		//以便立即从数据库中移走此条目。 
		CloseServiceHandle(scm);
	}
}


void  StartServer()
{
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scm != NULL){
		SC_HANDLE svc = OpenService(scm, SERVER_NAME, SERVICE_START);
		if (svc != NULL){
			StartService(svc, 0, NULL);//开始Service 
			CloseServiceHandle(svc);
		}
		CloseServiceHandle(scm);
	}
}


void  StopServer()
{
	SC_HANDLE scm = OpenSCManager(NULL, NULL,
		SC_MANAGER_ALL_ACCESS);
	if (scm != NULL){
		SC_HANDLE svc = OpenService(scm, SERVER_NAME,
			SERVICE_STOP | SERVICE_QUERY_STATUS);
		if (svc != NULL){
			SERVICE_STATUS ServiceStatus;
			QueryServiceStatus(svc, &ServiceStatus);
			if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
				ControlService(svc, SERVICE_CONTROL_STOP, &ServiceStatus);
			CloseServiceHandle(svc);
		}
		CloseServiceHandle(scm);
	}
}


void Server()
{
	SyncServer server;
	std::string server_ip;
	unsigned short server_port = 0;
	DBConnectInfo_t db_info;

	GetCfgInst()->GetServerInfo(server_ip, server_port);
	GetCfgInst()->GetDBInfo(db_info);
	server.SetServerInfo(server_ip, server_port);

	WriteToLog("Connect DB....");
	if(!server.InitDB(db_info))
	{	
		brun = false;
		WriteToLog("Connect DB Fail, Please Check Cfg");
		return;
		
	}
	
	WriteToLog("Connect DB Scuess");

	WriteToLog("begin server...");

	if(!server.Run())
	{

		brun = false;
		WriteToLog("Server Run Fail, Please Check Cfg");
		return;
	}

	WriteToLog("Server Run OK");

	while(brun)
	{
		st_sleep(10);
	}
}


void WINAPI ServiceMain(int argc, char** argv)
{

	st_init();
	servicestatus.dwServiceType = SERVICE_WIN32;

	servicestatus.dwCurrentState = SERVICE_START_PENDING;

	servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;//在本例中只接受系统关机和停止服务两种控制命令

	servicestatus.dwWin32ExitCode = 0;

	servicestatus.dwServiceSpecificExitCode = 0;

	servicestatus.dwCheckPoint = 0;

	servicestatus.dwWaitHint = 0;

	hstatus = ::RegisterServiceCtrlHandler(SERVER_NAME, CtrlHandler);

	if (hstatus==0)
	{
		WriteToLog("RegisterServiceCtrlHandler failed");
		return;
	}



	//向SCM 报告运行状态

	servicestatus.dwCurrentState = SERVICE_RUNNING;

	SetServiceStatus (hstatus, &servicestatus);

	//下面就开始任务循环了，你可以添加你自己希望服务做的工作

	brun=true;

	MEMORYSTATUS memstatus;


	GlobalMemoryStatus(&memstatus);
	int availmb=memstatus.dwAvailPhys/1024/1024;


	WriteToLog("********************Sync Server****************************");
	{		
		char str[128] = {};
		sprintf_s(str,100,"Available Memory is %dMB",availmb);
		WriteToLog(str);
	}

	{		
		char str[128] = {};
		int processID = GetCurrentProcessId();
		sprintf_s(str,100,"Current Process ID %d",processID);
		WriteToLog(str);
	}

	WriteToLog("****************************************************");
	
	Server();

	WriteToLog("stop server...");

	WriteToLog("service stopped");
}

void WINAPI CtrlHandler(DWORD request)

{

	switch (request)

	{

	case SERVICE_CONTROL_STOP:

		brun=false;
		STOP_SERVER = true;
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

	//printf("ok\n");
	//SMUSync sync;
	//sync.SetPlatAddr("www.home.see1000.com", 80);
	//sync.SetPlatAuthInfo("","");
	//sync.SetPlatInfo("","","");
	//sync.SetPlatAuthInfo("18280100206", "123456q");
	//sync.Login();
	Server();
	return NULL;
}

bool InitServer()
{
	char path[256] = {};
	ParseCfg* cfg = GetCfgInst();
	GetModuleFileName(NULL, path, 256);
	std::string strPath =path;
	int pos = strPath.find_last_of("\\");
	strPath = strPath.substr(0,pos+1);
	strPath.append("server.xcfg");
	bool flag = cfg->ReadCfg(strPath.c_str());
	if(!flag)
	{
		WriteToLog("读取配置文件server.xcfg失败,请检查配置文件是否有效!");
		return false;
	}
	return true;
}

void InstallServer()
{
	char path[256] = {};
	GetModuleFileName(NULL, path, 256);
	CreateServer(path);
	StartServer();
}

void CheckOneProcess()
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	//给系统内的所以进程拍一个快照
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		return ;
	}
	BOOL bMore = ::Process32First(hProcessSnap, &pe32);
	while (bMore)
	{

		if(!strcmp(pe32.szExeFile, "HMSyncPlatformServer.exe"))
		{
			WriteToLog("th32ProcessID = %x\n", pe32.th32ProcessID);
			if(brun)
			{

				//TerminateProcess(pe32.th32ProcessID, 0);
				WriteToLog("th32ProcessID = %x\n", pe32.th32ProcessID);
				printf("%s\n",pe32.szExeFile);
			}

		}
		bMore = ::Process32Next(hProcessSnap, &pe32);
	}
	::CloseHandle(hProcessSnap);

}

void TestMain()
{
	st_init();
	st_thread_create(handle_connect,0,0,0);
	st_sleep(1000000000000);

}

void main()
{
	if(!InitServer())
	{
		return;
	}
#if HM_SYNC_DEBUG
	TestMain();
#endif
	
	SERVICE_TABLE_ENTRY entrytable[2];

	entrytable[0].lpServiceName="HMSyncPlatformServer";

	entrytable[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)ServiceMain;

	entrytable[1].lpServiceName=NULL;

	entrytable[1].lpServiceProc=NULL;

	StartServiceCtrlDispatcher(entrytable);

	//InstallServer();
	printf("服务已经启动, 更多信息请查看日志，以确定服务是否正确运行!\n");
	return;
}