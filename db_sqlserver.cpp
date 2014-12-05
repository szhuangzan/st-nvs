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



typedef struct _wrap_db_oper_t
{
	st_dbfd_t*		dbfd;
	st_thread_t*	thread;
}wrap_db_oper_t;

st_msg_queue_t<wrap_db_oper_t> _st_db_msg_queue;
extern st_queue_t*	_st_lock_free_queue;
extern HANDLE	_st_notify_event;

HANDLE    _db_notify_event;

DWORD WINAPI db_server(LPVOID param)
{
	wrap_db_oper_t* db = (wrap_db_oper_t*)param;

	assert(db);
	st_dbfd_t* dbfd = db->dbfd;
	assert(dbfd);
	try
	{
		HRESULT hr;
		int rc=0;
		::CoInitialize(NULL);

		hr = dbfd->_pConn.CreateInstance(__uuidof(Connection));

		if (FAILED(hr))
		{
			dbfd->_oper_result = rc;
			//break;

		}

		hr = dbfd->_pRst.CreateInstance(__uuidof(Recordset));

		if (FAILED(hr)) 
		{
			dbfd->_pConn->Release();
			dbfd->_oper_result = rc;
			free(dbfd);
			dbfd = NULL;
			//break;
		}
		dbfd->_pConn->Open(dbfd->_connect_str, "","", adConnectUnspecified);
	}
	catch(_com_error& err)
	{
		printf("%s:%d -> %d,%s\n",__FUNCTION__,__LINE__,err.WCode(),(char*)err.Description());
	}

	_st_lock_free_enqueue(_st_lock_free_queue, db->thread);
	SetEvent(_st_notify_event);

	while(1)
	{
		Sleep(1000);
		DWORD result = WaitForSingleObject(_db_notify_event, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
		
		}
	}

	return 0;
}

APIEXPORT st_dbfd_t* st_db_connect(const char* connect_str)
{
	int rc = 0;
	st_dbfd_t* dbfd = (st_dbfd_t*)calloc(1, sizeof(st_dbfd_t));
	do
	{


#if 0 
		// Start using the Connection events
		hr = dbfd->_pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&dbfd->_pCPC);

		if (FAILED(hr)) 
		{
			dbfd->_oper_result = rc;
			return dbfd;
		}

		hr = dbfd->_pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &dbfd->_pCP);
		dbfd->_pCPC->Release();

		if (FAILED(hr)) 
			return rc;

		_pConnEvent = new CConnEvent();
		hr = _pConnEvent->QueryInterface(__uuidof(IUnknown), (void **) &_pUnk);

		if (FAILED(hr)) 
			return rc;

		hr = _pCP->Advise(_pUnk, &_dwConnEvt);
		_pCP->Release();

		if (FAILED(hr)) 
			return rc;

		 Start using the Recordset events
		hr = dbfd->_pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&dbfd->_pCPC);

		if (FAILED(hr)) 
		{
			dbfd->_oper_result = rc;
			return dbfd;
		}

		hr = dbfd->_pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &dbfd->_pCP);
		dbfd->_pCPC->Release();

		if (FAILED(hr)) 
		{
			dbfd->_oper_result = rc;
			return dbfd;
		}

		_pRstEvent = new CRstEvent();
		hr = _pRstEvent->QueryInterface(__uuidof(IUnknown), (void **) &_pUnk);

		if (FAILED(hr)) 
			return rc;

		hr = _pCP->Advise(_pUnk, &_dwRstEvt);
		_pCP->Release();

		if (FAILED(hr)) 
		{
			dbfd->_oper_result = rc;
			return dbfd;
		}
#endif
		wrap_db_oper_t* db = (wrap_db_oper_t*)malloc(sizeof(wrap_db_oper_t));

		dbfd->_connect_str = _strdup(connect_str);
		db->dbfd = dbfd;
		db->thread = _ST_CURRENT_THREAD();

		HANDLE handle =  CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)db_server, (LPVOID)db, 0, 0);
		if(handle)
		{
			CloseHandle(handle);
		}

		_st_wait(-1);

	}while(0);

	return dbfd;
}


APIEXPORT 