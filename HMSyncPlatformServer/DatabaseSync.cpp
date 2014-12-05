#include "DatabaseSync.h"

#import "C:\Program Files (x86)\Common Files\System\ado\msado15.dll" no_namespace rename("EOF", "EndOfFile")
// The Connection events
class CConnEvent : public ConnectionEventsVt {
private:
	ULONG m_cRef;

public:
	CConnEvent() { m_cRef = 0; };
	~CConnEvent() {};

	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP raw_InfoMessage( struct Error *pError, 
		EventStatusEnum *adStatus, 
	struct _Connection *pConnection);

	STDMETHODIMP raw_BeginTransComplete( LONG TransactionLevel,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Connection *pConnection);

	STDMETHODIMP raw_CommitTransComplete( struct Error *pError, 
		EventStatusEnum *adStatus,
	struct _Connection *pConnection);

	STDMETHODIMP raw_RollbackTransComplete( struct Error *pError, 
		EventStatusEnum *adStatus,
	struct _Connection *pConnection);

	STDMETHODIMP raw_WillExecute( BSTR *Source,
		CursorTypeEnum *CursorType,
		LockTypeEnum *LockType,
		long *Options,
		EventStatusEnum *adStatus,
	struct _Command *pCommand,
	struct _Recordset *pRecordset,
	struct _Connection *pConnection);

	STDMETHODIMP raw_ExecuteComplete( LONG RecordsAffected,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Command *pCommand,
	struct _Recordset *pRecordset,
	struct _Connection *pConnection);

	STDMETHODIMP raw_WillConnect( BSTR *ConnectionString,
		BSTR *UserID,
		BSTR *Password,
		long *Options,
		EventStatusEnum *adStatus,
	struct _Connection *pConnection);

	STDMETHODIMP raw_ConnectComplete( struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Connection *pConnection);

	STDMETHODIMP raw_Disconnect( EventStatusEnum *adStatus, struct _Connection *pConnection);
};

// The Recordset events
class CRstEvent : public RecordsetEventsVt {
private:
	ULONG m_cRef;   

public:
	CRstEvent() { m_cRef = 0; };
	~CRstEvent() {};

	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP raw_WillChangeField( LONG cFields,      
		VARIANT Fields,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_FieldChangeComplete( LONG cFields,
		VARIANT Fields,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_WillChangeRecord( EventReasonEnum adReason,
		LONG cRecords,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_RecordChangeComplete( EventReasonEnum adReason,
		LONG cRecords,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_WillChangeRecordset( EventReasonEnum adReason,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_RecordsetChangeComplete( EventReasonEnum adReason,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_WillMove( EventReasonEnum adReason,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_MoveComplete( EventReasonEnum adReason,
	struct Error *pError,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_EndOfRecordset( VARIANT_BOOL *fMoreData,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_FetchProgress( long Progress,
		long MaxProgress,
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);

	STDMETHODIMP raw_FetchComplete( struct Error *pError, 
		EventStatusEnum *adStatus,
	struct _Recordset *pRecordset);
};

// Implement each connection method
STDMETHODIMP CConnEvent::raw_InfoMessage( struct Error *pError, 
										 EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_BeginTransComplete( LONG TransactionLevel,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_CommitTransComplete( struct Error *pError,
												 EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_RollbackTransComplete( struct Error *pError,
												   EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_WillExecute( BSTR *Source,
										 CursorTypeEnum *CursorType,
										 LockTypeEnum *LockType,
										 long *Options,
										 EventStatusEnum *adStatus,
struct _Command *pCommand,
struct _Recordset *pRecordset,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_ExecuteComplete( LONG RecordsAffected,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Command *pCommand,
struct _Recordset *pRecordset,
struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_WillConnect( BSTR *ConnectionString,
										 BSTR *UserID,
										 BSTR *Password,
										 long *Options,
										 EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());;
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_ConnectComplete( struct Error *pError,
											 EventStatusEnum *adStatus,
struct _Connection *pConnection) {
	printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CConnEvent::raw_Disconnect( EventStatusEnum *adStatus, struct _Connection *pConnection) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

// Implement each recordset method
STDMETHODIMP CRstEvent::raw_WillChangeField( LONG cFields,
											VARIANT Fields,
											EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_FieldChangeComplete( LONG cFields,
												VARIANT Fields,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_WillChangeRecord( EventReasonEnum adReason,
											 LONG cRecords,
											 EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_RecordChangeComplete( EventReasonEnum adReason,
												 LONG cRecords,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_WillChangeRecordset( EventReasonEnum adReason,
												EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_RecordsetChangeComplete( EventReasonEnum adReason,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_WillMove( EventReasonEnum adReason, 
									 EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_MoveComplete( EventReasonEnum adReason,
struct Error *pError,
	EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_EndOfRecordset( VARIANT_BOOL *fMoreData,
										   EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_FetchProgress( long Progress,
										  long MaxProgress,
										  EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::raw_FetchComplete( struct Error *pError,
										  EventStatusEnum *adStatus,
struct _Recordset *pRecordset) {
	*adStatus = adStatusUnwantedEvent;
	return S_OK;
};

STDMETHODIMP CRstEvent::QueryInterface(REFIID riid, void ** ppv) {
	*ppv = NULL;
	if (riid == __uuidof(IUnknown) || riid == __uuidof(RecordsetEventsVt)) 
		*ppv = this;

	if (*ppv == NULL)
		return ResultFromScode(E_NOINTERFACE);

	AddRef();
	return NOERROR;
}

STDMETHODIMP_(ULONG) CRstEvent::AddRef() { 
	return ++m_cRef; 
};

STDMETHODIMP_(ULONG) CRstEvent::Release() { 
	if (0 != --m_cRef) 
		return m_cRef;
	delete this;
	return 0;
}

STDMETHODIMP CConnEvent::QueryInterface(REFIID riid, void ** ppv) {
	*ppv = NULL;
	if (riid == __uuidof(IUnknown) || riid == __uuidof(ConnectionEventsVt)) 
		*ppv = this;

	if (*ppv == NULL)
		return ResultFromScode(E_NOINTERFACE);

	AddRef();
	return NOERROR;
}

STDMETHODIMP_(ULONG) CConnEvent::AddRef() { 
	return ++m_cRef; 
};

STDMETHODIMP_(ULONG) CConnEvent::Release() { 
	if (0 != --m_cRef) 
		return m_cRef;
	delete this;
	return 0;
}


///

DatabaseSync::DatabaseSync(void)
:_pCPC(NULL),
	_pCP(NULL),
	_pUnk(NULL),
	_pRstEvent(NULL),
	_pConnEvent(NULL)

{
}

DatabaseSync::~DatabaseSync(void)
{
}


int DatabaseSync::Init()
{
	HRESULT hr;
	int rc = 0;

	::CoInitialize(NULL);

	hr = _pConn.CreateInstance(__uuidof(Connection));

	if (FAILED(hr)) 
		return rc;

	hr = _pRst.CreateInstance(__uuidof(Recordset));

	if (FAILED(hr)) 
		return rc;

	// Start using the Connection events
	hr = _pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&_pCPC);

	if (FAILED(hr)) 
		return rc;

	hr = _pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &_pCP);
	_pCPC->Release();

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

	// Start using the Recordset events
	hr = _pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&_pCPC);

	if (FAILED(hr)) 
		return rc;

	hr = _pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &_pCP);
	_pCPC->Release();

	if (FAILED(hr)) 
		return rc;

	_pRstEvent = new CRstEvent();
	hr = _pRstEvent->QueryInterface(__uuidof(IUnknown), (void **) &_pUnk);

	if (FAILED(hr)) 
		return rc;

	hr = _pCP->Advise(_pUnk, &_dwRstEvt);
	_pCP->Release();

	if (FAILED(hr)) 
		return rc;
	return rc;
}

int DatabaseSync::UnInit()
{
	HRESULT hr = 0;
	int rc = 0;
	_pRst->Close();
	_pConn->Close();

	// Stop using the Connection events
	hr = _pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &_pCPC);

	if (FAILED(hr)) 
		return rc;

	hr = _pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &_pCP);
	_pCPC->Release();

	if (FAILED(hr)) 
		return rc;

	hr = _pCP->Unadvise(_dwConnEvt );
	_pCP->Release();

	if (FAILED(hr)) 
		return rc;

	// Stop using the Recordset events
	hr = _pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &_pCPC);

	if (FAILED(hr)) 
		return rc;

	hr = _pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &_pCP);
	_pCPC->Release();

	if (FAILED(hr)) 
		return rc;

	hr = _pCP->Unadvise(_dwRstEvt );
	_pCP->Release();

	if (FAILED(hr)) 
		return rc;

	CoUninitialize();
	return rc;
}

void DatabaseSync::SetDBConnectInfo(const DBConnectInfo_t&)
{

}


bool DatabaseSync::Connect()
{
	bool flag = true;
	char buf[1024] = {};
	sprintf_s(buf, "Provider=SQLOLEDB;Server=%s;Database=%s;User ID=%s;Password=%s;Data Source=%s,%d",
					_db_connect_info._server_ip,
					_db_connect_info._database_name,
					_db_connect_info._dbms_user_name,
					_db_connect_info._dbms_user_pwd,
					_db_connect_info._server_ip,
					_db_connect_info._server_port);

	try
	{
		_pConn->Open(buf, "","", adConnectUnspecified);
	}
	catch(_com_error& err)
	{
		printf("%s:%d -> %s\n",__FUNCTION__,__LINE__,(char*)err.Description());
		flag = false;
	}
	
	return flag;
}

bool DatabaseSync::ReadDBSyncSerialNo()
{
	bool flag = true;

	return flag;
}

bool DatabaseSync::WriteDBSyncNoUpdateSerialNo(std::string msg)
{
	bool flag = true;

	return flag;
}

bool DatabaseSync::DisConnect()
{
	bool flag = true;

	return flag;
}