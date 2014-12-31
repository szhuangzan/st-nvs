#pragma once
#include<string>
#include "st.h"
#include "SMUSync.h"
typedef struct _DBConnectInfo_t
{
	std::string				_server;
	std::string				_server_ip;
	std::string				_database_name;
	std::string				_dbms_user_name;
	std::string				_dbms_user_pwd;
	unsigned short			_server_port;
	_DBConnectInfo_t()
	{

	}
}DBConnectInfo_t;


class CRstEvent	;
class CConnEvent;

class DatabaseSync
{
public:
	DatabaseSync(void);
	~DatabaseSync(void);
	
public:
	void SetDBConnectInfo(const DBConnectInfo_t&);
	bool Connect();
	bool ReadDBSyncSerialNo();
	bool CreateUserByDB(const SyncPlatformData&, SyncPlatformDataResult& result);
	bool UpdateUserStateByDB(const SyncPlatformData& data, SyncPlatformDataResult& resul);
	bool UpdateUserByDB(const SyncPlatformData& data, SyncPlatformDataResult& resul);
	bool DisConnect();
private:
	bool CheckState();
	bool GetPlatName(const std::string& userID, std::string& platName);
	bool GetUserState(const std::string& userID, std::string& platName);
private:
	char	          _connect_str[2014];
	st_dbfd_t		  _dbfd;

};
