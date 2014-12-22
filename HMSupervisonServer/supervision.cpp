#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <time.h>
#pragma comment(lib, "WS2_32.lib")

#define	HM_SYNC_DEBUG 0
#define LOG_MAX_SIZE 128
		

#define SERVER_NAME			 "HMSyncPlatformServer"
#define SUPERVISION_NAME	 "HMSupervisionServer"



SERVICE_STATUS servicestatus;

SERVICE_STATUS_HANDLE hstatus;


void WINAPI ServiceMain(int argc, char** argv);

void WINAPI CtrlHandler(DWORD request);

bool	brun=false;


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
		_log_filename.append("super.log");
		_log_file = fopen(_log_filename.c_str(), "w+");
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

		printf("%s\n", buf);
#else
		i_Write(log.c_str());
#endif
		return;
	}
private:
	void timestamp(char* buf)
	{
		char cur_time[64] = {};
		time_t cur =0;
		time(&cur);
		struct tm* cur_tm  = localtime(&cur);
		sprintf_s(cur_time, "%04d-%02d-%02d %02d:%02d:%02d ->",
			cur_tm->tm_year+1900, 
			cur_tm->tm_mon+1,
			cur_tm->tm_mday,
			cur_tm->tm_hour,
			cur_tm->tm_min,
			cur_tm->tm_sec);
		sprintf_s(buf, 64,"%s ", cur_time);
	}
	void i_Write(const char* log)
	{

		if (!_log_file)
		{
			_log_file = fopen(_log_filename.c_str(), "r+");
		}


		{
			if(fwrite(log, 1, strlen(log), _log_file) == 0)
			{
				fclose(_log_file);
				_log_file = fopen(_log_filename.c_str(), "r+");

			}

			fflush(_log_file);

		}

		{
			_file_size = ftell(_log_file);

			if(_file_size >= LOG_MAX_SIZE*1024)
			{
				int pos = fseek(_log_file, 0, SEEK_SET);
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
		char buf[2048];
		va_list arg_list;
		va_start(arg_list, fmt);
		vsnprintf(buf, 1024, fmt, arg_list);
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
			int err = GetLastError();
			if(ERROR_SERVICE_DOES_NOT_EXIST == err)
			{
				WriteToLog("%s Not Exist", SERVER_NAME);
				flag = true;	
			}
		}
		CloseServiceHandle(scm);
	}
	return flag;
}



void   CreateServer(const char* server_path)
{
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
			WriteToLog("Create %s",SERVER_NAME);
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

SERVICE_STATUS GetServerStatus()
{
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scm != NULL){
		SC_HANDLE svc = OpenService(scm, SERVER_NAME,
			SERVICE_ALL_ACCESS);
		if (svc != NULL){
			
			QueryServiceStatus(svc, &ServiceStatus);
			CloseServiceHandle(svc); //删除Service后，最好再调用CloseServiceHandle 

		}
		//以便立即从数据库中移走此条目。 
		CloseServiceHandle(scm);
	}
	return ServiceStatus;
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


void CheckOneProcess()
{
	bool is_run = false;
	char ServerExe[128] = {};
	sprintf_s(ServerExe,128,"%s.exe",SERVER_NAME);
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
		
		if(!strcmp(pe32.szExeFile, ServerExe))
		{
			WriteToLog("%s ProcessID  = %d Running\n",ServerExe, pe32.th32ProcessID);
			is_run = true;
		}
		bMore = ::Process32Next(hProcessSnap, &pe32);
	}
	::CloseHandle(hProcessSnap);
	if(!is_run)
	{
		StopServer();
	}
}

void Server()
{
	brun = true;
	char path[MAX_PATH] = {};
	std::string strPath;
	{
		GetModuleFileName(NULL, path, MAX_PATH);
		 strPath = path;
		int pos = strPath.find_last_of("\\");
		strPath = strPath.substr(0,pos+1);
		strPath.append(SERVER_NAME);
	}
	while(brun)
	{
	
		if(CheckNotServer())
		{
			CreateServer(strPath.c_str());
		}

		{
			SERVICE_STATUS ServiceStatus = GetServerStatus();
			if(ServiceStatus.dwCurrentState!=SERVICE_RUNNING && 
				ServiceStatus.dwCurrentState!=SERVICE_START_PENDING )
				StartServer();
		}
		
		CheckOneProcess();
		Sleep(10000);
	}
}


void WINAPI ServiceMain(int argc, char** argv)
{

	servicestatus.dwServiceType = SERVICE_WIN32;

	servicestatus.dwCurrentState = SERVICE_START_PENDING;

	servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;//在本例中只接受系统关机和停止服务两种控制命令

	servicestatus.dwWin32ExitCode = 0;

	servicestatus.dwServiceSpecificExitCode = 0;

	servicestatus.dwCheckPoint = 0;

	servicestatus.dwWaitHint = 0;

	hstatus = ::RegisterServiceCtrlHandler(SUPERVISION_NAME, CtrlHandler);

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


	WriteToLog("********************Super Server****************************");
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


void main()
{
#if HM_SYNC_DEBUG
	Server();
#endif
	
	SERVICE_TABLE_ENTRY entrytable[2];

	entrytable[0].lpServiceName= SUPERVISION_NAME;

	entrytable[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)ServiceMain;

	entrytable[1].lpServiceName=NULL;

	entrytable[1].lpServiceProc=NULL;

	StartServiceCtrlDispatcher(entrytable);
	printf("服务已经启动, 更多信息请查看日志，以确定服务是否正确运行!\n");
	return;
}