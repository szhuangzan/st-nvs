// database.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


// ADO_Events_Model_Example.cpp
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
											  printf("%x\n",GetCurrentThreadId());
   *adStatus = adStatusUnwantedEvent;
   return S_OK;
};

STDMETHODIMP CConnEvent::raw_ConnectComplete( struct Error *pError,
                                              EventStatusEnum *adStatus,
                                              struct _Connection *pConnection) {
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
											  printf("SSSSSSSSS");
   *adStatus = adStatusUnwantedEvent;
   return S_OK;
};

STDMETHODIMP CRstEvent::raw_EndOfRecordset( VARIANT_BOOL *fMoreData,
                                            EventStatusEnum *adStatus,
                                            struct _Recordset *pRecordset) {
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

int main() {
   HRESULT hr;
   DWORD dwConnEvt;
   DWORD dwRstEvt;
   IConnectionPointContainer *pCPC = NULL;
   IConnectionPoint *pCP = NULL;
   IUnknown *pUnk = NULL;
   CRstEvent *pRstEvent = NULL;
   CConnEvent *pConnEvent= NULL;
   int rc = 0;
   _RecordsetPtr pRst; 
   _ConnectionPtr pConn;

   ::CoInitialize(NULL);

   hr = pConn.CreateInstance(__uuidof(Connection));

   if (FAILED(hr)) 
      return rc;

   hr = pRst.CreateInstance(__uuidof(Recordset));

   if (FAILED(hr)) 
      return rc;

   // Start using the Connection events
   hr = pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&pCPC);

   if (FAILED(hr)) 
      return rc;
   
   hr = pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &pCP);
   pCPC->Release();

   if (FAILED(hr)) 
      return rc;

   pConnEvent = new CConnEvent();
   hr = pConnEvent->QueryInterface(__uuidof(IUnknown), (void **) &pUnk);

   if (FAILED(hr)) 
      return rc;
   
   hr = pCP->Advise(pUnk, &dwConnEvt);
   pCP->Release();

   if (FAILED(hr)) 
      return rc;

   // Start using the Recordset events
   hr = pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **)&pCPC);

   if (FAILED(hr)) 
      return rc;

   hr = pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &pCP);
   pCPC->Release();

   if (FAILED(hr)) 
      return rc;

   pRstEvent = new CRstEvent();
   hr = pRstEvent->QueryInterface(__uuidof(IUnknown), (void **) &pUnk);

   if (FAILED(hr)) 
      return rc;

   hr = pCP->Advise(pUnk, &dwRstEvt);
   pCP->Release();

   if (FAILED(hr)) 
      return rc;
   printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
   // Do some work
   pConn->Open("Provider=SQLOLEDB;Server=182.131.21.104;Database=alarm;User ID=alarm_user;Password=Huamaitel.com0822;Data Source=182.131.21.104,3199", "", "", adAsyncConnect);
    printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
	Sleep(1000);
	try
	{
   pRst->Open("SELECT DeviceSN,DeviceName FROM HM_Host", (IDispatch *) pConn, adOpenStatic, adLockReadOnly, adCmdText|adAsyncExecute);
	}
	catch(_com_error& e)
	{	
		 printf("%d thread id : %x %s\n", __LINE__,GetCurrentThreadId(),(char*)e.Description());
		
	}
Sleep(1000);
   pRst->MoveFirst();
    printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
   while (pRst->EndOfFile == FALSE) {
      wprintf(L"DeviceSN = '%s'\n", (wchar_t*) ((_bstr_t) pRst->Fields->GetItem("DeviceSN")->Value));
      pRst->MoveNext();
   }

    printf("%d thread id : %x\n", __LINE__,GetCurrentThreadId());
   pRst->Close();
   pConn->Close();

   // Stop using the Connection events
   hr = pConn->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &pCPC);

   if (FAILED(hr)) 
      return rc;

   hr = pCPC->FindConnectionPoint(__uuidof(ConnectionEvents), &pCP);
   pCPC->Release();

   if (FAILED(hr)) 
      return rc;

   hr = pCP->Unadvise( dwConnEvt );
   pCP->Release();

   if (FAILED(hr)) 
      return rc;

   // Stop using the Recordset events
   hr = pRst->QueryInterface(__uuidof(IConnectionPointContainer), (void **) &pCPC);

   if (FAILED(hr)) 
      return rc;

   hr = pCPC->FindConnectionPoint(__uuidof(RecordsetEvents), &pCP);
   pCPC->Release();

   if (FAILED(hr)) 
      return rc;

   hr = pCP->Unadvise( dwRstEvt );
   pCP->Release();

   if (FAILED(hr)) 
      return rc;

   CoUninitialize();
}