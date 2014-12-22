#include "DatabaseSync.h"
#include <string>
extern int WriteToLog(char* str,...);
DatabaseSync::DatabaseSync()
:_dbfd(0)
{

}

DatabaseSync::~DatabaseSync()
{

}

void DatabaseSync::SetDBConnectInfo(const DBConnectInfo_t& info)
{
	memset(_connect_str,0,sizeof(_connect_str));
	if(info._server_port == 1433)
	{
		sprintf_s(_connect_str, 1024, "Provider=SQLOLEDB;Server=%s;Database=%s;User ID=%s;Password=%s;Data Source=%s",
			info._server.c_str(),
			info._database_name.c_str(),
			info._dbms_user_name.c_str(),
			info._dbms_user_pwd.c_str(),
			info._server_ip.c_str()
			);
	}
	else
	{
		sprintf_s(_connect_str, 1024, "Provider=SQLOLEDB;Server=%s;Database=%s;User ID=%s;Password=%s;Data Source=%s,%d",
			info._server.c_str(),
			info._database_name.c_str(),
			info._dbms_user_name.c_str(),
			info._dbms_user_pwd.c_str(),
			info._server_ip.c_str(),
			info._server_port
			);
	}

}

bool DatabaseSync::Connect()
{
	bool flag = true;
	int err_code = 0;
	char buf[128] = {};
	_dbfd = 0;
	_dbfd = st_db_connect(_connect_str, &err_code, buf, sizeof(buf));
	if(err_code != 0)
	{
		WriteToLog("DB Connect Error = %x, ErrDesc = %s", err_code, buf);
	}
	if(!_dbfd) flag = false;
	return flag;
}


bool DatabaseSync::CreateUserByDB(const SyncPlatformData& data, SyncPlatformDataResult& result)
{
	bool		flag = true;
	char		sql[512] = {};
	char create_user_time[20] = {};
	{
		time_t cur_time = st_time();
		struct tm cur_tm ={};
		localtime_s(&cur_tm, &cur_time);
		sprintf_s(create_user_time, 20, "%04d-%02d-%02d",cur_tm.tm_year+1900, cur_tm.tm_mon+1, cur_tm.tm_mday);

		sprintf_s(sql,512, "insert into [HM_User](UserID,AreaID,Address, UserName, CreateUserTime, SerialNumber, UserState) values('%s','%s','%s','%s','%s','%s','%s')", 
					data.CustID.c_str(),
					"root",
					data.LinkAddr.c_str(),
					data.CustName.c_str(),
					create_user_time,
					data.SerialNumber.c_str(),
					data.CustState.c_str());
	}

	{
		char desc[128] = {};
		int ErrCode = 0;
		if(_dbfd)
		{
			if((ErrCode = st_db_exec(_dbfd, sql, desc, sizeof(desc)))!=0)
			{
				result.Desc = desc;
				if(ErrCode == 0x80040e2f){
					result.ErrCode = "1";
				}
				else{
					result.ErrCode = "2";
				}
				
				flag  = false;
				WriteToLog("Create User, ErrCode = %x, ErrDesc = %s", ErrCode, result.Desc.c_str())	;
			}
		}
		else
		{
			result.Desc = "DB Error";
			result.ErrCode = "2";
		}

	}

	return flag;
}

bool DatabaseSync::UpdateUserStateByDB(const SyncPlatformData& data, SyncPlatformDataResult& result)
{

	char desc[128] = {};
	int ErrCode = 0;
	do 
	{

		{
			char sql[1024] = {};
			sprintf_s(sql, "select PlatLoginName from HM_User  where UserID='%s'", 
				data.CustID.c_str());
			ErrCode = st_db_query(_dbfd, sql, desc, sizeof(desc));
			if(ErrCode)
			{
				WriteToLog("Update User, ErrCode = %x, ErrDesc = %s", ErrCode, result.Desc.c_str())	;
				result.ErrCode = "3";
				break;
			}

			std::vector<std::wstring> platLogin;
			ErrCode = st_db_fetch(_dbfd, "PlatLoginName", platLogin, desc, sizeof(desc));
			if(ErrCode)
			{
				result.ErrCode = "3";
				break;
			}
			if(platLogin.size()==1)
			{
				const std::wstring& wstr = platLogin[0];
				std::string str(wstr.length(), ' ');
				std::copy(wstr.begin(), wstr.end(), str.begin());
				result.ViewName = str;
			}
		}

		{
			char sql[1024] = {};
			sprintf_s(sql, "update HM_User set UserState = '%s' where UserID='%s'", 
				data.CustState.c_str(),
				data.CustID.c_str());

			if((ErrCode = st_db_exec(_dbfd, sql, desc, sizeof(desc)))!=0)
			{
				WriteToLog("Update User State, ErrCode = %x, ErrDesc = %s", ErrCode, result.Desc.c_str())	;
				result.ErrCode = "3";
				break;
			}
		
		}

	} while (0);

	if(ErrCode)
	{
		result.Desc = desc;
		WriteToLog("UpdateUserStateByDB, ErrCode = %d, ErrDesc = %s", result.ErrCode, result.Desc.c_str())	;
	}
	return !ErrCode;
}


bool DatabaseSync::UpdateUserByDB(const SyncPlatformData& data, SyncPlatformDataResult& result)
{
	bool flag = true;
	char sql[1024] = {};

	{
		sprintf_s(sql, "update HM_User set UserName= '%s' where UserID='%s'", 
			data.CustName.c_str(),
			data.CustID.c_str());
	}

	{
		char desc[128] = {};
		int ErrCode = 0;
		if((ErrCode = st_db_exec(_dbfd, sql, desc, sizeof(desc)))!=0)
		{
			result.ErrCode = "3";
			result.Desc = desc;
			flag  = false;
			WriteToLog("Update User, ErrCode = %d, ErrDesc = %s", result.ErrCode, result.Desc.c_str())	;
		}
	}
	return flag;
}


bool DatabaseSync::DisConnect()
{
	bool flag = true;
	st_db_close(_dbfd);
	_dbfd = 0;
	return flag;
}