/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is Mozilla Communicator client code, released
* March 31, 1998.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation. Portions created by Netscape are
* Copyright (C) 1998-1999 Netscape Communications Corporation. All
* Rights Reserved.
*
* Portions created by SGI are Copyright (C) 2000 Silicon Graphics, Inc.
* All Rights Reserved.
*
* Contributor(s): Silicon Graphics, Inc.
*
* Alternatively, the contents of this file may be used under the terms
* of the ____ license (the  "[____] License"), in which case the provisions
* of [____] License are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the [____]
* License and not to allow others to use your version of this file under the
* NPL, indicate your decision by deleting  the provisions above and replace
* them with the notice and other provisions required by the [____] License.
* If you do not delete the provisions above, a recipient may use your version
* of this file under either the NPL or the [____] License.
*/

/*
* This file is derived directly from Netscape Communications Corporation,
* and consists of extensive modifications made during the year(s) 1999-2000.
*/

#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "common.h"

#include "lock_queue.h"


enum {DB_OPEN, DB_QUERY, DB_FETCH, DB_EXEC, DB_CLOSE};



extern st_queue_t*	_st_lock_free_queue;
extern HANDLE	_st_notify_event;


class SqlServerDBOper
{
public:
	SqlServerDBOper(st_dbfd_t* dbfd)
		:_dbfd(dbfd)
	{

	}
	~SqlServerDBOper()
	{
		Close();
	}
public:

	bool Open()
	{
		bool flag = true;
		try
		{
			do 
			{

				HRESULT hr;
				int rc=0;
				::CoInitialize(NULL);

				hr = _dbfd->_pConn.CreateInstance(__uuidof(Connection));

				if (FAILED(hr))
				{
					_dbfd->_err_code = rc;
					flag = false;
					break;
				}

				hr = _dbfd->_pRst.CreateInstance(__uuidof(Recordset));

				if (FAILED(hr)) 
				{
					_dbfd->_pConn->Release();
					_dbfd->_err_code = rc;
					flag = false;
					break;
				}
				_dbfd->_pConn->PutConnectionTimeout(5);
				_dbfd->_pConn->PutCommandTimeout(2);
				_dbfd->_pConn->Open(_dbfd->_connect_str, "","", adConnectUnspecified);
			} while (0);
		}
		catch(_com_error& err)
		{
			HandleErr(err);
			flag =false;
		}
		return flag;
	}

	int Close()
	{
		HRESULT hr;
		int		rc = 0;

		hr = _dbfd->_pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &_dbfd->_pCPC);

		if (FAILED(hr)) 
			return rc;

		hr = _dbfd->_pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &_dbfd->_pCP);
		_dbfd->_pCPC->Release();

		if (FAILED(hr)) 
			return rc;

		// Stop using the Recordset events
		hr = _dbfd->_pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &_dbfd->_pCPC);

		if (FAILED(hr)) 
			return rc;

		hr = _dbfd->_pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &_dbfd->_pCP);
		_dbfd->_pCPC->Release();

		if (FAILED(hr)) 
			return rc;

		CoUninitialize();
		return 0;
	}

	bool Query(const char* sql)
	{
		bool flag = true;
		try
		{
			if(_dbfd->_pRst->GetState() != adStateClosed) _dbfd->_pRst->Close();
			_dbfd->_pRst->Open(sql, (IDispatch *) _dbfd->_pConn, adOpenStatic, adLockReadOnly, adCmdText);
			try
			{
				_dbfd->_err_code  = 0;
				_dbfd->_pRst->MoveFirst();
				//while (_dbfd->_pRst->EndOfFile == FALSE) 
				//{
				//	//_dbfd->_fetch_result.push_back((wchar_t*) ((_bstr_t)_dbfd->_pRst->Fields->GetItem("UserID")->Value));
				//	_dbfd->_pRst->MoveNext();
				//}
			}
			catch(_com_error& err)
			{
				HandleErr(err);
				CheckDBConnect(err.Error());
				flag = false;
			}
		}
		catch(_com_error& err)
		{	
			HandleErr(err);
			CheckDBConnect(err.Error());
			flag = false;
		}
		return flag;
	}

	bool Fetch(const char* item)
	{
		bool flag = true;
		try
		{
			if(_dbfd->_pRst->GetState() != adStateOpen) 
			{
				flag = false;
				return flag;
			}
			_dbfd->_pRst->MoveFirst();
			while (_dbfd->_pRst->EndOfFile == FALSE) 
			{
				_dbfd->_fetch_result.push_back((wchar_t*) ((_bstr_t)_dbfd->_pRst->Fields->GetItem(item)->Value));
				_dbfd->_pRst->MoveNext();
			}
			_dbfd->_pRst->Close();
		}
		catch(_com_error& err)
		{	
			HandleErr(err);
			flag = false;
		}
		return flag;
	}

	bool Exec(const char* sql)
	{
		bool flag = true;
		try
		{
			_dbfd->_err_code = 0;
			_dbfd->_pConn->Execute((_bstr_t)sql, NULL, adCmdText);
		}
		catch(_com_error& err)
		{
			HandleErr(err);
			CheckDBConnect(err.Error());
			flag = false;
		}
		return flag;
	}


private:
	void HandleErr(_com_error& err)
	{
		_dbfd->_err_code = err.Error();
		memset(_dbfd->_err_desc, 0, sizeof(_dbfd->_err_desc));
		sprintf_s(_dbfd->_err_desc, sizeof(_dbfd->_err_desc)-1, "%s", (char*)err.Description());
	}

	void CheckDBConnect(int err)
	{
		if ((err == 0x80004005)||(err == 0x800a0e7d)||(err == 0x800a0e78))
		{
			
			try{
				if (_dbfd->_pConn->GetState() != adStateClosed)	_dbfd->_pConn->Close();
			
				_dbfd->_pConn->Open(_dbfd->_connect_str,"","",NULL);
			}
			catch(_com_error& err)
			{
				HandleErr(err);
			}
		}
	}
private:
	st_dbfd_t*			_dbfd;
};

DWORD WINAPI db_server(LPVOID param)
{
	bool	exit = false;
	st_dbfd_t* dbfd = (st_dbfd_t*)param;

	SqlServerDBOper sqlServerDB(dbfd);

	while(!exit)
	{
		DWORD result = dbfd->_msg_queue->wait(INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			wrap_db_oper_t* oper = 0;
			dbfd->_msg_queue->dequeue(oper);
			if(oper && oper->event == DB_OPEN)
			{
				if(!sqlServerDB.Open())
				{
					exit = true;
				}
			}
			else if(oper && oper->event == DB_QUERY)
			{
				sqlServerDB.Query(oper->buf);
			}
			else if(oper && oper->event == DB_FETCH)
			{
				sqlServerDB.Fetch(oper->buf);
				exit = true;
			}
			else if(oper && oper->event == DB_EXEC)
			{
				sqlServerDB.Exec(oper->buf);
			}
			else if(oper && oper->event == DB_CLOSE)
			{
				sqlServerDB.Close();
				exit = true;
			}

			if(oper)
			{
				_st_lock_free_enqueue(_st_lock_free_queue, oper->thread);
				SetEvent(_st_notify_event);
				free((void*)oper);
			}
		}
	}

	return 0;
}

APIEXPORT st_dbfd_t* st_db_connect(const char* connect_str, int* err_code, char* err_desc, int desc_len)
{
	int rc = 0;
	st_dbfd_t* dbfd = (st_dbfd_t*)calloc(1, sizeof(st_dbfd_t));
	dbfd->_msg_queue = new st_msg_queue_t<wrap_db_oper_t*>();
	do
	{
		wrap_db_oper_t* db = (wrap_db_oper_t*)calloc(1, sizeof(wrap_db_oper_t));

		dbfd->_connect_str = _strdup(connect_str);
		db->event = DB_OPEN;
		db->thread = _ST_CURRENT_THREAD();

		HANDLE handle =  CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)db_server, (LPVOID)dbfd, 0, 0);
		if(handle)
		{
			CloseHandle(handle);
		}
		dbfd->_err_code = 0;
		dbfd->_msg_queue->enqueue(db);
		dbfd->_msg_queue->notify();
		_st_wait(-1);

	}while(0);

	*err_code = dbfd->_err_code;
	strcpy_s(err_desc, desc_len, dbfd->_err_desc);

	if(dbfd->_err_code)
	{
		free(dbfd->_connect_str);
		delete dbfd->_msg_queue;
		free(dbfd);
		dbfd = 0;
	}
	return dbfd;
}


APIEXPORT int st_db_query(st_dbfd_t* dbfd, const char* sql, char* desc, int desc_len)
{
	wrap_db_oper_t* db = (wrap_db_oper_t*)calloc(1, sizeof(wrap_db_oper_t));
	db->thread = _ST_CURRENT_THREAD();
	db->event  = DB_QUERY;
	strcpy_s(db->buf, sql);
	dbfd->_err_code = 0;
	dbfd->_msg_queue->enqueue(db);
	dbfd->_msg_queue->notify();
	_st_wait(-1);

	strcpy_s(desc,  desc_len, dbfd->_err_desc);

	return dbfd->_err_code;
}

APIEXPORT int st_db_fetch(st_dbfd_t* dbfd, const char* item, std::vector<std::wstring>& result, char* desc, int desc_len)
{
	wrap_db_oper_t* db = (wrap_db_oper_t*)calloc(1, sizeof(wrap_db_oper_t));
	db->thread = _ST_CURRENT_THREAD();
	db->event  = DB_FETCH;
	strcpy_s(db->buf, item);
	dbfd->_err_code = 0;
	dbfd->_msg_queue->enqueue(db);
	dbfd->_msg_queue->notify();
	
	_st_wait(-1);

	for(int i = 0;i<dbfd->_fetch_result.size();i++)
		result.push_back(dbfd->_fetch_result[i]);

	strcpy_s(desc,  desc_len, dbfd->_err_desc);

	return dbfd->_err_code;
}



APIEXPORT int  st_db_exec(st_dbfd_t* dbfd, const char* sql, char* desc, int desc_len)
{
	wrap_db_oper_t* db = (wrap_db_oper_t*)calloc(1, sizeof(wrap_db_oper_t));
	db->thread = _ST_CURRENT_THREAD();
	db->event  = DB_EXEC;
	dbfd->_err_code = 0;
	strcpy_s(db->buf, sql);
	dbfd->_msg_queue->enqueue(db);
	dbfd->_msg_queue->notify();
	_st_wait(-1);
	
	strcpy_s(desc,  desc_len, dbfd->_err_desc);
	return dbfd->_err_code;
}

APIEXPORT int st_db_close(st_dbfd_t* dbfd)
{
	wrap_db_oper_t* db = (wrap_db_oper_t*)calloc(1, sizeof(wrap_db_oper_t));
	db->thread = _ST_CURRENT_THREAD();
	db->event  = DB_CLOSE;
	dbfd->_err_code = 0;
	dbfd->_msg_queue->enqueue(db);
	dbfd->_msg_queue->notify();
	_st_wait(-1);
	delete dbfd->_msg_queue;
	free(dbfd);
	return 0;
}