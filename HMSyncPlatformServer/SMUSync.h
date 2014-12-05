#pragma once
#include <BaseTsd.h>
#include <string>
extern "C"
{
#include "st.h"
};


class SMUSync
{
public:
	SMUSync(void);
	~SMUSync(void);

	void  SetPlatAddr(const char* url, const UINT16 port);
	void  SetPlatAuthInfo(const char* UserName, const char* Password);
	void  SetPlatInfo(const char* PlatType, const char* PlatSofVer, const char* PlatHardVer);

	bool Login();
	bool SyncFromPlatToDB();
	bool SyncDBFromPlat();
private:
	void PacketWebService(const char* action, std::string& Body);
	std::string ParseWebService(const char* web, const char* compare_str);
private:
	std::string   _PlatUrl;
	std::string   _PlatIp;
	UINT16        _PlatPort;
	std::string   _PlatUserName;
	std::string   _PlatPassword;
	std::string	  _PlatType;
	std::string   _PlatSoftVer;
	std::string   _PlatHardVer;
	std::string	  _PlatCookie;

	std::string   _Sync_To_Msg;
	std::string	  _Sync_From_Msg;

	st_netfd_t*	  _srv_fd;
};

