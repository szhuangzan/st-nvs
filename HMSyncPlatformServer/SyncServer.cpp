#include "SyncServer.h"
#include "st.h"
#include "ParseCfg.h"
#define  SYNC_PLAT_TIME 60 

extern int WriteToLog(char* fmt, ...);
typedef struct _SyncSession
{
	st_netfd_t*				_SessionFd;
	DatabaseSync*			_SyncDB;
}SyncSession;


DatabaseSync*	SyncServer::_DBSync = 0;

SyncServer::SyncServer(void)
:_server_fd()
{
}

SyncServer::~SyncServer(void)
{

}


void SyncServer::SetServerInfo(const std::string& server_ip, UINT16 port)
{
	_server_ip = server_ip;
	_server_port = port;
	char buf[1024] = {};
	return;
}

bool SyncServer::InitDB(const DBConnectInfo_t& info)
{
	if(!_DBSync) _DBSync = new DatabaseSync;
	 _DBSync->SetDBConnectInfo(info);
	 return _DBSync->Connect();
}

bool SyncServer::Run()
{
	bool flag = true;
	flag = CreateListen();
	if(flag)
	{
		st_thread_create(HandleConnectTask, _server_fd,0,0);
	}
	else
	{
		int rc = WSAGetLastError();
		WriteToLog("%s:%d server fail , err = %d", _server_ip.c_str(),_server_port, rc);
	}
	return flag;
}

bool SyncServer::CreateListen()
{
	bool flag =true;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(_server_port);
	serv_addr.sin_addr.s_addr = inet_addr(_server_ip.c_str());

	_server_fd = st_netfd_listen(serv_addr);
	if(!_server_fd) flag = false;

	return flag;
}

void* SyncServer::HandleConnectTask(void* arg)
{
	st_netfd_t srvfd = (st_netfd_t*)arg;
	st_netfd_t clifd = 0;

	struct sockaddr_in from = {};
	int fromlen = sizeof(from);
	char buf[1024] = {};
	{
		clifd = st_accept(srvfd, buf, ((sizeof(SOCKADDR_IN)+16) * 2), -1);
		if (clifd == NULL) 
		{
			st_thread_create(HandleConnectTask, (void *)srvfd, 0, 0);
			return NULL;
		}
		st_thread_create(HandleConnectTask, srvfd, 0, 0);
		st_thread_create(HandleSessionTask, clifd, 0, 0);
	}
	return NULL;
}

void* SyncServer::HandleSessionTask(void* arg)
{
	std::string status_code = "200 OK";
	st_netfd_t fd = (st_netfd_t*)arg;
	if(!fd) return NULL;

	SMUSync sync;
	std::vector<SyncPlatformData> data_vec;
	std::vector<SyncPlatformDataResult> result_vec;
	do 
	{
		if(!sync.ParseWebService(fd, data_vec))
		{
			status_code = "404";
			break;
		}

		
		for(unsigned i=0;i<data_vec.size();i++)
		{
			SyncPlatformDataResult  result;
			result.SerialNumber = data_vec[i].SerialNumber;
			result.ErrCode = "0";
			result.Desc = "";
			if(!data_vec[i].CustState.compare("0101"))
			{
				if(!_DBSync->CreateUserByDB(data_vec[i], result))
				{
					result.ErrCode = "1";
				}
			}
			else if(!data_vec[i].CustState.compare("0102"))	//用户变更
			{
				if(!_DBSync->UpdateUserByDB(data_vec[i], result))
				{
					result.ErrCode = "1";
				}
			}
			else if(!data_vec[i].CustState.compare("0103"))
			{
				if(!_DBSync->UpdateUserStateByDB(data_vec[i], result))
				{
					result.ErrCode = "3";
				}				
			}
			else
			{
				result.ErrCode = "4";
				result.Desc = "No Support Cmd";
			}
			result.UserState = data_vec[i].CustState;
			result_vec.push_back(result);
			
			WriteToLog("Oper = %s, ErrCode = %s, ErrDesc = %s", result.UserState.c_str(), result.ErrCode.c_str(), result.Desc.c_str());
		}

	} while (0);

	sync.PacketWebService(fd, status_code.c_str(), result_vec);
	st_netfd_close(fd);
	return NULL;
}


void* SyncServer::UpdateUserInfoToPlatTask(void* arg)
{
	sockaddr_in PlatAddr ;
	st_netfd_t  fd = 0;
	SMUSync		sync;
	const PlatLoginInfo& PlatInfo = GetCfgInst()->GetPlatLoginInfo();
	{
		ZeroMemory (&PlatAddr , sizeof (sockaddr_in ));
		PlatAddr.sin_family = AF_INET ;
		PlatAddr.sin_addr .s_addr = inet_addr (PlatInfo.PlatUrl.c_str());
		PlatAddr.sin_port = htons (PlatInfo.PlatPort);
	}

	WriteToLog("UpdateUserInfoToPlatTask Begin...");
	do 
	{
		fd =  st_connect(PlatAddr, sizeof(sockaddr_in), -1);
		if(!fd) continue;

		if(!sync.LoginPlat(fd, PlatInfo)) continue;

		if(!sync.SendUserStateToPlat(""))   continue;
		
		st_netfd_close(fd);
		st_sleep(SYNC_PLAT_TIME);
	} while (1);
	
	WriteToLog("UpdateUserInfoToPlatTask End...");
	return NULL;
}
