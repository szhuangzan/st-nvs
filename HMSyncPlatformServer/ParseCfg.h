#pragma once
#include "xml.hpp"
#include "DatabaseSync.h"

class ParseCfg
{
public:
	ParseCfg(void);
	~ParseCfg(void);
public:
	bool ReadCfg(const char* cfg);
	bool GetServerInfo(std::string& server_ip, unsigned short& server_port);
	bool GetDBInfo(DBConnectInfo_t& db);
	PlatLoginInfo&  GetPlatLoginInfo();
private:

	std::string				_server_ip;
	unsigned short			_server_port;
	DBConnectInfo_t			_db_connect_info;
	PlatLoginInfo			_plat_login_info;
private:

};

extern ParseCfg* GetCfgInst();

