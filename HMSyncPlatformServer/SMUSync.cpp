#include "xml.hpp"
#include "SMUSync.h"
#include <WinSock2.h>

extern "C"
{
#include "st.h"
};

#define REQUEST_TIMEOUT 60
#define SEC2USEC(s) ((s)*1000000LL)

SMUSync::SMUSync(void)
{
}

SMUSync::~SMUSync(void)
{
}

void SMUSync::SetPlatAddr(const char* url, const UINT16 port)
{
	hostent* host = gethostbyname(url);
	char buf[20] ={};
	sprintf_s(buf, 20, "%d.%d.%d.%d",
			host->h_addr_list[0][0]&0xff,
			host->h_addr_list[0][1]&0xff,
			host->h_addr_list[0][2]&0xff,
			host->h_addr_list[0][3]&0xff);
	_PlatIp = buf;
	_PlatUrl = url;
	_PlatPort = port;
}

void SMUSync::SetPlatAuthInfo(const char* UserName, const char* Password)
{
	_PlatUserName = UserName;
	_PlatPassword = Password;
}

void SMUSync::SetPlatInfo(const char* PlatType, const char* PlatSofVer, const char* PlatHardVer)
{
	_PlatHardVer = PlatHardVer;
	_PlatType = PlatType;
	_PlatSoftVer = PlatSofVer;
}

void SMUSync::PacketWebService(const char* action, std::string& Body)
{
	char buf[20] = {};

	_Sync_To_Msg.clear();

	_Sync_To_Msg += "POST /Service/Clientex.asmx HTTP/1.1\r\n";

	_Sync_To_Msg += "Host: ";
	
	_Sync_To_Msg += _PlatUrl;

	_Sync_To_Msg += "\r\n";
	_Sync_To_Msg += "Content-Type: text/xml; charset=utf-8\r\n";
	_Sync_To_Msg += "Content-Length: ";

	UINT32 strlen = Body.length();
	sprintf_s(buf, sizeof(strlen), "%d", strlen);
	_Sync_To_Msg += buf;
	
	_Sync_To_Msg += "\r\n";


	_Sync_To_Msg += "SOAPAction: \"http://see1000.com/service/";

	_Sync_To_Msg += action;
	_Sync_To_Msg += "\"";
	_Sync_To_Msg += "\r\n";
	_Sync_To_Msg += "User-Agent: ";
	_Sync_To_Msg += _PlatType;
	_Sync_To_Msg += " ";
	_Sync_To_Msg += _PlatHardVer;
	_Sync_To_Msg += " ";
	_Sync_To_Msg += _PlatSoftVer;
	if(!_PlatCookie.empty())
	{
		_Sync_To_Msg += "\r\n";
		_Sync_To_Msg += "Cookie: ";
		_Sync_To_Msg += _PlatCookie;
	}
	_Sync_To_Msg += "\r\n";
	_Sync_To_Msg += "\r\n";
	_Sync_To_Msg += Body;
}


bool SMUSync::Login()
{
	DMXml xml;
	bool exit = false;
	std::string body;
	char		RecvBuf[2048] = {};
	UINT32      RecvLen = 0;

	{
		sockaddr_in addrPeer ;  
		ZeroMemory (&addrPeer , sizeof (sockaddr_in ));
		addrPeer.sin_family = AF_INET ;
		addrPeer.sin_addr .s_addr = inet_addr (_PlatIp.c_str());
		addrPeer.sin_port = htons ( _PlatPort );
		_srv_fd =  st_connect(addrPeer, sizeof(sockaddr_in),-1);
		if(!_srv_fd)
		{
			return false;
		}
	}

	xml.NewRoot("soap:Envelope")
		->SetTextAttribute("xmlns:soap", "http://www.w3.org/2003/05/soap-envelope")
		->SetTextAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
		->SetTextAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema")
		->SetTextAttribute("xmlns:soapenc", "http://schemas.xmlsoap.org/soap/encoding/")
		->NewChild("soap:Body", 0)
		->NewChild("Authenticate", 0)
		->SetTextAttribute("xmlns","http://see1000.com/service")
		->NewChild("name", _PlatUserName.c_str())->GetParent()
		->NewChild("pass", _PlatPassword.c_str());
	body = xml.Encode();

	PacketWebService("Authenticate", body);

	if (!_srv_fd || st_write(_srv_fd, _Sync_To_Msg.c_str(), _Sync_To_Msg.length(),SEC2USEC(REQUEST_TIMEOUT))<0)
	{
		exit = true;
	}

	if (!_srv_fd||(RecvLen = st_read(_srv_fd, RecvBuf, 2048, SEC2USEC(REQUEST_TIMEOUT)))<0)
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);
		exit = true;
	}

	printf("recv buf = %s\n",RecvBuf);
	{
		std::string result = ParseWebService(RecvBuf, "AuthenticateResult");
		if(result.compare("true"))
		{
			return false;
		}
	}
	return true;
}

std::string SMUSync::ParseWebService(const char* web, const char* compare_str)
{
	std::string WebInfo = web;

	UINT32 headlen = 0;
	UINT32 bodylen = 0;
	INT32 pos = WebInfo.find("\r\n\r\n");
	if(pos > 0)
	{
		std::string header = WebInfo.substr(0, pos);
		headlen = header.size();
		pos = header.find("Content-Length:");
		if(pos > 0)
		{
			INT32 endpos = header.find("\r\n", pos);
			INT32 phrlen = endpos - pos;
			std::string contlen = header.substr(pos, phrlen);
			pos = contlen.find_first_of(':');
			std::string temp = contlen.substr(pos + 1, phrlen - pos - 1);
			bodylen = atoi(temp.c_str());
		}
		if(headlen > 0)
		{
			INT32 l = headlen + 2 + bodylen;
		}
	}

	{
		INT32 pos = WebInfo.find("Cookie:");
		INT32 endpos = WebInfo.find("\r\n", pos);
		if(pos > 0)
		{
			INT32 sid_pos = WebInfo.find("ASP.NET_SessionId", pos);
			if(sid_pos == WebInfo.npos) _PlatCookie = WebInfo.substr(pos + 7, endpos - pos - 7);
		}
	}

	if(WebInfo.size() > 10)
	{
		if(!WebInfo.substr(WebInfo.size() - 10, 10).compare(":Envelope>") ||
			!WebInfo.substr(WebInfo.size() - 7, 7).compare("</html>"))
		{
		
		}
	}

	{
		std::string com = compare_str;
		INT32 begpos = WebInfo.find(com);
		if(begpos == std::string::npos)
		{
			return false;
		}
		begpos += strlen(compare_str)+1;

		com.insert(0,1,'/');
		INT32 endpos = WebInfo.find(com);
		if(endpos == std::string::npos)
		{
			return "";
		}

		endpos -= 1;
		std::string result = WebInfo.substr(begpos, endpos-begpos);
		printf("%s\n",result.c_str());
		return result;
	}

}