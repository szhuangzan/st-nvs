#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>     //΢����չ�����

#pragma  comment(lib,"Mswsock.lib")
#pragma comment(lib, "WS2_32.lib")

using namespace std;
#define SEND 0
#define RECV 1
#define ACCEPT 2

#define DATA_LENGTH 1000
//��������ݶ���
typedef struct _PER_HANDLE_DATA
{
	SOCKET socket;     //��ص��׽���
	SOCKADDR_STORAGE clientAddr;     //�ͻ��˵ĵ�ַ
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//��IO��������
typedef struct{
	OVERLAPPED overlapped;
	WSABUF buffer;     //һ�����ݻ�����,����WSASend/WSARecv�еĵڶ�������
	char dataBuffer[DATA_LENGTH];     //ʵ�ʵ����ݻ�����
	int dataLength;                     //ʵ�ʵ����ݻ���������
	int operatorType;                 //��������,����ΪSEND/RECV����
	SOCKET client;                     //�ֱ��ʾ���͵��ֽ����ͽ��յ��ֽ���
}PER_IO_DATA,*LPPER_IO_DATA;

typedef struct 
{
	HANDLE CompletionPort;
	SOCKET Listen;
}ListenComplete;

extern void accept_ex(SOCKET Listen);
DWORD WINAPI ServerThread(LPVOID lpParam)
{
	ListenComplete* ListenWrap = (ListenComplete*)lpParam;
	HANDLE CompletionPort = ListenWrap->CompletionPort;
	DWORD bytes;
	LPPER_HANDLE_DATA perHandleData = NULL;     //���������
	LPPER_IO_DATA perIoData;             //��IO����
	DWORD Flags;
	int ret;
	DWORD RecvBytes;
	//����ѭ���ĵȴ��ص����������
	int clientCount = 0;
	while(true)
	{
		bytes = -1;
		cout<<" wait for data arrive..."<<endl;
		ret=GetQueuedCompletionStatus(
			CompletionPort,                 //ԭ�ȵ���ɶ˿ھ��
			&bytes,                         //�ص�������ɵ��ֽ���
			(LPDWORD)&perHandleData,     //ԭ�Ⱥ���ɶ˿ھ�����������ĵ��������
			(LPOVERLAPPED*)&perIoData,     //���ڽ�������ɵ�IO�������ص��ṹ
			INFINITE);                     //����ɶ˿��ϵȴ���ʱ�� INFINITE Ϊ���޵ȴ�
	cout<<"COMPLETE "<<perIoData->operatorType<<endl;
	cout<<"perHandleData "<<hex<<perHandleData<<" perIoData "<<hex<<perIoData<<" SOCK "<<perHandleData->socket<<endl;
	cout<<"perIoData->overlapped "<<hex<<&perIoData->overlapped<<endl;
		//�ȼ�����׽������Ƿ�������
		//����������ʱ�ر��׽���ͬʱ�ͷŵ����е��ڴ�.
		int i = 0;
		if(bytes == 0 && (perIoData->operatorType == RECV ||
			perIoData->operatorType == SEND))
		{
			closesocket(perHandleData->socket);
			GlobalFree(perHandleData);
			GlobalFree(perIoData);
			cout<<"closesocket and globalfree perhandledata periodata!"<<endl;
			continue;
		}
		//����AcceptEx����������ɣ������洦��
		if(perIoData->operatorType == ACCEPT)     //�������Ӳ���
		{
			cout<<"CINNECT..."<<endl;

			//ʹ��GetAcceptExSockaddrs���� ��þ���ĸ�����ַ����.
			if(setsockopt( perIoData->client, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,  
				( char* )&(perHandleData->socket ), sizeof( perHandleData->socket ) )==SOCKET_ERROR)
				cout<<"setsockopt..."<<endl;

			perHandleData->socket = perIoData->client;

			//memcpy(&(perHandleData->clientAddr),raddr,sizeof(raddr));
			//���µĿͻ��׽�������ɶ˿�����
			CreateIoCompletionPort((HANDLE)perHandleData->socket,
				CompletionPort,(ULONG_PTR)perHandleData,0);

			memset(&(perIoData->overlapped),0,sizeof(OVERLAPPED));
			perIoData->operatorType = RECV;         //��״̬���óɽ���
			//����WSABUF�ṹ
			perIoData->buffer.buf = perIoData->dataBuffer;
			perIoData->buffer.len = perIoData->dataLength = DATA_LENGTH;

			cout<<"wait for data arrive(Accept)..."<<endl;
			Flags = 0;
			HANDLE event; // ���Ҫ����¼������ﵱȻҲ��Ҫһ��WSAEVENT����
			// ��Ҫע����ǿ���һ��SOCKETͬʱ����һ�����ϵ��ص����� Ҳ�ͻ��Ӧһ�����ϵ�WSAEVENT
			event =WSACreateEvent();
			perIoData->overlapped.hEvent=event;

			if(WSARecv(perHandleData->socket,&(perIoData->buffer),1,
				&RecvBytes,&Flags,&(perIoData->overlapped),NULL)==SOCKET_ERROR)
				if(WSAGetLastError()==WSA_IO_PENDING)
					cout<<"WSARecv Pending..."<<endl;
			accept_ex(ListenWrap->Listen);
			continue;            
		}
		if(perIoData->operatorType == RECV)
			cout<<perIoData->buffer.buf<<endl;     //�����յ���������ʾ����
		Flags = 0;
		perIoData->operatorType = RECV;             //���óɽ�����������

		ZeroMemory(&(perIoData->overlapped),sizeof(OVERLAPPED));
		//����Ͷ��һ���µĽ�������
		
		WSARecv(perHandleData->socket,&(perIoData->buffer),1,
			&RecvBytes,&Flags,&(perIoData->overlapped),NULL);
	} 

	return 0;
}

void accept_ex(SOCKET Listen)
{

	LPFN_ACCEPTEX lpfnAcceptEx = NULL;     //AcceptEx����ָ��
	//Accept function GUID
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	//get acceptex function pointer
	DWORD dwBytes = 0;
	if(WSAIoctl(Listen,SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,sizeof(guidAcceptEx),&lpfnAcceptEx,sizeof(lpfnAcceptEx),
		&dwBytes,NULL,NULL)==0)
		cout<<"WSAIoctl success..."<<endl;
	else{
		cout<<"WSAIoctl failed..."<<endl;
		switch(WSAGetLastError())
		{
		case WSAENETDOWN:
			cout<<""<<endl;
			break;
		case WSAEFAULT:
			cout<<"WSAEFAULT"<<endl;
			break;
		case WSAEINVAL:
			cout<<"WSAEINVAL"<<endl;
			break;
		case WSAEINPROGRESS:
			cout<<"WSAEINPROGRESS"<<endl;
			break;
		case WSAENOTSOCK:
			cout<<"WSAENOTSOCK"<<endl;
			break;
		case WSAEOPNOTSUPP:
			cout<<"WSAEOPNOTSUPP"<<endl;
			break;
		case WSA_IO_PENDING:
			cout<<"WSA_IO_PENDING"<<endl;
			break;
		case WSAEWOULDBLOCK:
			cout<<"WSAEWOULDBLOCK"<<endl;
			break;
		case WSAENOPROTOOPT:
			cout<<"WSAENOPROTOOPT"<<endl;
			break;
		}
		return;
	}

	//while(true)
	//{
	//׼������ AcceptEx �������ú���ʹ���ص��ṹ������ɶ˿�����
	LPPER_IO_DATA perIoData = (LPPER_IO_DATA)GlobalAlloc(GPTR,sizeof(PER_IO_DATA));
	memset(&(perIoData->overlapped),0,sizeof(OVERLAPPED));    
	perIoData->operatorType = ACCEPT;
	//��ʹ��AcceptExǰ��Ҫ�����ؽ�һ���׽���������ڶ�������������Ŀ���ǽ�ʡʱ��
	//ͨ�����Դ���һ���׽��ֿ�
	perIoData->client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);

	perIoData->dataLength = DATA_LENGTH;
	DWORD flags = 0;

	//����AcceptEx��������ַ������Ҫ��ԭ�е��������16���ֽ�
	//ע������ʹ�����ص�ģ�ͣ��ú�������ɽ�������ɶ˿ڹ����Ĺ����߳��д���
	cout<<"Process AcceptEx function wait for client connect..."<<endl;
	int rc = lpfnAcceptEx(Listen,perIoData->client,perIoData->dataBuffer,
		perIoData->dataLength-((sizeof(SOCKADDR_IN)+16)*2),
		sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,&dwBytes,
		&(perIoData->overlapped));
	if(rc == FALSE)
	{
		if(WSAGetLastError()!=ERROR_IO_PENDING)
			cout<<"lpfnAcceptEx failed.."<<endl;
	}
}

void main()
{
	HANDLE CompletionPort;
	WSADATA data;
	SYSTEM_INFO info;
	SOCKADDR_IN addr;
	SOCKET Listen;

	unsigned int i;
	WSAStartup(MAKEWORD(2,2),&data);

	//����һ��IO��ɶ˿�
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
	//ȷ��������������
	GetSystemInfo(&info);     //�����߳�

	//����һ�������׽���(�����ص�����)
	Listen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
	for(i=0;i<1;i++)
	{
		//���ݴ�����������������Ӧ��Ĵ����߳�
		ListenComplete* ListenWrap = new ListenComplete;
		ListenWrap->Listen = Listen;
		ListenWrap->CompletionPort=CompletionPort;
		HANDLE thread = CreateThread(NULL,0,ServerThread,ListenWrap,0,NULL);
		CloseHandle(thread);
	}
	//�������׽�������ɶ˿ڰ�
	LPPER_HANDLE_DATA perDandleData;
	perDandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR,sizeof(PER_HANDLE_DATA));
	perDandleData->socket = Listen;
	CreateIoCompletionPort((HANDLE)Listen,CompletionPort,(ULONG_PTR)perDandleData,0);

	cout<<"listen perDandleData "<<hex<<perDandleData<<endl;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(5500);

	bind(Listen,(PSOCKADDR)&addr,sizeof(addr));
	listen(Listen,5);

	accept_ex(Listen);

	
	cin>>i;
	closesocket(Listen);
	WSACleanup();
}