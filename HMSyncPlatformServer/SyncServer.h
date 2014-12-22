#pragma once
#include"SMUSync.h"
#include "DatabaseSync.h"


class SyncServer
{
public:
	SyncServer(void);
	~SyncServer(void);
public:
	void SetServerInfo(const std::string& server_ip, UINT16 port);
	bool InitDB(const DBConnectInfo_t& info);
	bool Run();
	bool Close();
private:
	bool CreateListen();
	bool ParseWebInfo(const char* info);
	bool PacketWebInfo(const char* info);
private:
	static void* HandleConnectTask(void* arg);
	static void* HandleSessionTask(void* arg);
	static void* UpdateUserInfoToPlatTask(void* arg);
private:
	std::string				_server_ip;
	UINT16					_server_port;
	st_netfd_t				_server_fd;

	static DatabaseSync*	_DBSync;

};
