#pragma once
#include <string>
#include <vector>

#include "st.h"
#include "xml.hpp"

typedef struct _SyncPlatformData
{
	std::string		SerialNumber;
	std::string		CustID;
	std::string		CustAccunt;
	std::string		CustName;
	std::string		CustState;
	std::string		AreaID;
	std::string		LinkAddr;

}SyncPlatformData;

typedef struct _SyncPlatformDataResult
{
	std::string		SerialNumber;
	std::string		ErrCode;
	std::string		Desc;
	std::string     UserState;
	std::string     ViewName;
}SyncPlatformDataResult;

typedef struct _PlatLoginInfo
{
	std::string		PlatUrl;
	std::string		PlatUserName;
	std::string		PlatPwd;
	unsigned short	PlatPort;

}PlatLoginInfo;

class SMUSync
{
public:
	SMUSync(void);
	~SMUSync(void);
	bool ParseWebService(st_netfd_t, std::vector<SyncPlatformData>& data);
	bool PacketWebService(st_netfd_t, const char* status_code, const std::vector<SyncPlatformDataResult>& result);

	bool LoginPlat(st_netfd_t fd, const PlatLoginInfo& plat);
	bool SendUserStateToPlat(const std::string& user);

	static wchar_t* SMUSync::Utf8ToUnicode(const char* utf);
	static char* SMUSync::UnicodeToUtf8(const char* unicode);
	static char* SMUSync::UnicodeToAscii(const char* unicode);
private:
	int ReadInfo(st_netfd_t fd, std::string& buf);
	bool GetValueByName(std::string& msg, const char* name, std::string& value);
	bool ParseData(DMXml& xml, SyncPlatformData& data);

//	bool PacketPlatServer(const char* action, std::string& Body);

};

