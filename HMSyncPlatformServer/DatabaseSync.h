#pragma once
#include<string>
#include <OCIdl.h>
#import "C:\Program Files (x86)\Common Files\System\ado\msado15.dll" no_namespace rename("EOF", "EndOfFile")

typedef struct _DBConnectInfo_t
{
	std::string		_server_ip;
	std::string		_database_name;
	std::string		_dbms_user_name;
	std::string		_dbms_user_pwd;
	UINT16			_server_port;
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
	int Init();
	int UnInit();
	void SetDBConnectInfo(const DBConnectInfo_t&);
	bool Connect();
	bool ReadDBSyncSerialNo();
	bool WriteDBSyncNoUpdateSerialNo(std::string msg);	
	bool DisConnect();
private:
	bool CheckState();
private:
	DBConnectInfo_t	  _db_connect_info;

	IConnectionPointContainer	*_pCPC;
	IConnectionPoint			*_pCP;
	IUnknown					*_pUnk;
	CRstEvent					*_pRstEvent;
	CConnEvent					*_pConnEvent;
	_RecordsetPtr				_pRst; 
	_ConnectionPtr				_pConn;
	DWORD						_dwConnEvt;
	DWORD						_dwRstEvt;
};
