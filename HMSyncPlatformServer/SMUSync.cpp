
#include "SMUSync.h"



#define REQUEST_TIMEOUT 60
#define SEC2USEC(s) ((s)*1000000LL)

extern int WriteToLog(char* fmt, ...);

SMUSync::SMUSync(void)
{
}

SMUSync::~SMUSync(void)
{
}


bool SMUSync::ParseWebService(st_netfd_t fd, std::vector<SyncPlatformData>& data_vec)
{
	std::string body;
	ReadInfo(fd, body);
	
	if(body.empty())
	{
		return false;
	}

	{
		DMXml xml;
		xml.Decode(body.c_str());
		
		xml.GetRoot();
		if(!xml.CheckNodeValid())
		{
			WriteToLog("Xml Error");
			return false;
		}

		{
			SyncPlatformData data;
			xml.GetRoot()->FindElement("SyncData");
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				return false;
			}
			if (ParseData(xml, data))
			{
				data_vec.push_back(data);
			}
		}

		while(xml.FindAllElement("SyncData"))
		{
			SyncPlatformData data;
			if (ParseData(xml, data))
			{
				data_vec.push_back(data);
			}
		}
	}
	return true;

}

bool SMUSync::PacketWebService(st_netfd_t fd, const char* status_code, const std::vector<SyncPlatformDataResult>& result)
{

	std::string body;
	std::string head;
	std::string msg;
	{
		if(result.size() != 0)
		{
			DMXml xml;
			
			xml.NewRoot("SyncResponseResult");
			for(int i =0;i<result.size();i++)
			{
				xml.NewChild("SyncResult",0)->NewChild("StreamNo",result[i].SerialNumber.c_str())->GetParent()
					->NewChild("Result", result[i].ErrCode.c_str())->GetParent()
					->NewChild("Descr", result[i].Desc.c_str())->GetParent()
					->NewChild("OPFlag",result[i].UserState.c_str())->GetParent()
					->NewChild("ViewerName",result[i].ViewName.c_str())->GetParent();
				xml.GetParent();
			}
			body = xml.Encode();
		}
		else
		{
			body = "<body>This is HMSyncPlatformServer, Is Running!</body>";
		}
	}


	char body_len[10] = {};
	sprintf_s(body_len, " %d\r\n",body.length());

	head = "HTTP/1.1 ";
	head.append(status_code);
	head.append("\r\n");
	head.append("Server: HMSyncPlatformServer\r\n");
	head.append("Content-Type: text/html\r\n");
	head.append("Accept-Ranges: byte\r\n");
	head.append("Connection: close\r\n");
	head.append("Content-Length: ");
	head.append(body_len);
	head.append("\r\n");

	msg.append(head);
	msg.append(body);

	st_write(fd, msg.c_str(), msg.length()+1, -1);
	return true;
}

bool SMUSync::GetValueByName(std::string& msg, const char* name, std::string& value)
{
	int pos = msg.find(name);
	if(pos == std::string::npos) return false;

	{
		std::string tmp1 = msg.substr(pos, msg.length()-pos);
		int colon_pos = tmp1.find(":");
		if(colon_pos == std::string::npos) return false;
		int comma_pos = tmp1.find(":");
		if(comma_pos == std::string::npos) return false;

		value = tmp1.substr(colon_pos, comma_pos-colon_pos-1);
	}
	return true;
}

int SMUSync::ReadInfo(st_netfd_t fd, std::string& body)
{

	int total_len = 0;
	int read_len = 0;
	std::string WebInfo ;
	{
		char buf[2049] = {};
		read_len = st_read(fd, buf, 2047, -1);
		if(read_len < 0)
		{
			return false;
		}

		WebInfo.clear();
		WebInfo = buf;
	}

	UINT32 headlen = 0;
	UINT32 bodylen = 0;
	INT32 body_pos = WebInfo.find("\r\n\r\n");
	if(body_pos > 0)
	{
		std::string header = WebInfo.substr(0, body_pos);
		headlen = header.size();
		int pos = header.find("Content-Length:");
		if(pos > 0)
		{
			INT32 endpos = header.find("\r\n", pos);
			INT32 phrlen = endpos - pos;
			std::string contlen = header.substr(pos, phrlen);
			pos = contlen.find_first_of(':');
			std::string temp = contlen.substr(pos + 1, phrlen - pos - 1);
			bodylen = atoi(temp.c_str());
		}
		else
		{
			return 0;
		}
		if(headlen > 0)
		{
			total_len = headlen  + bodylen+sizeof("\r\n\r\n")-1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
	
	

	if(WebInfo.find("POST") == std::string::npos)
	{
		body = "";

		return 0;
	}

	if(read_len < total_len)
	{
		char* buf = (char*)calloc(1, total_len-read_len);
		int len = st_read_fully(fd, buf,total_len-read_len, -1);
		read_len += len;
		WebInfo.append(buf);
		free(buf);
	}
	body = WebInfo.substr(body_pos+sizeof("\r\n\r\n")-1);
	return read_len;
}

bool SMUSync::ParseData(DMXml& xml, SyncPlatformData& data)
{
	bool flag = true;
	do
	{
		{
			char* tmp =  xml.FindElement("StreamNo")->GetValueText();
			if(tmp)
			{
				data.SerialNumber  = tmp;
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;
			}
		}



		{
			char* tmp = xml.GetParent()->FindElement("OPFlag")->GetValueText();
			if(tmp)
			{
				data.CustState  = tmp;
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;
			}
		}
		{
			char* tmp = xml.GetParent()->FindElement("CustID")->GetValueText();
			if(tmp)
			{
				data.CustID  = tmp;
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;
			}
		}

		{
			char* tmp = xml.GetParent()->FindElement("CustAccount")->GetValueText();
			if(tmp)
			{
				data.CustAccunt   = tmp;
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;;
			}
		}
		
		{
			char* tmp = xml.GetParent()->FindElement("CustName")->GetValueText();
			if(tmp)
			{
				std::wstring wstr;
				wchar_t* unciode = Utf8ToUnicode(tmp);
				char* ascii = UnicodeToAscii((char*)unciode);
				data.CustName = ascii;
				free(ascii);
				free(unciode);
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;
			}
		}

		{
			char* tmp = xml.GetParent()->FindElement("InstalledAddress")->GetValueText();
			if(tmp)
			{
				
				wchar_t* unciode = Utf8ToUnicode(tmp);
				char* ascii = UnicodeToAscii((char*)unciode);
			
				data.LinkAddr = ascii;
				free(ascii);
				free(unciode);
				free(tmp);
			}
			if(!xml.CheckNodeValid())
			{
				WriteToLog("Xml Error");
				flag = false;
				break;
			}
		}
	}while(0);

	xml.GetParent();
	return flag;
}

bool SMUSync::LoginPlat(st_netfd_t fd, const PlatLoginInfo& plat)
{
	DMXml xml;
	bool flag = true;
	std::string msg;
	char		RecvBuf[2048] = {};
	UINT32      RecvLen = 0;

	xml.NewRoot("soap:Envelope")
		->SetTextAttribute("xmlns:soap", "http://www.w3.org/2003/05/soap-envelope")
		->SetTextAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
		->SetTextAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema")
		->SetTextAttribute("xmlns:soapenc", "http://schemas.xmlsoap.org/soap/encoding/")
		->NewChild("soap:Body", 0)
		->NewChild("Authenticate", 0)
		->SetTextAttribute("xmlns","http://see1000.com/service")
		->NewChild("name", plat.PlatUserName.c_str())->GetParent()
		->NewChild("pass", plat.PlatPwd.c_str());
	std::string body = xml.Encode();

	//PacketPlatServer("Authenticate", body);

	do 
	{
		if (st_write(fd, msg.c_str(), msg.length(),-1)<0)
		{
			flag = true;
			break;
		}

		if (st_read(fd, RecvBuf, 2048, -1)<0)
		{
			flag = false;
			break;
		}

	/*	{
			std::string result = ParseWebService(RecvBuf, "AuthenticateResult");
			if(result.compare("true"))
			{
				return false;
			}
		}*/

	} while (0);

	return flag;
}


bool SMUSync::SendUserStateToPlat(const std::string& user)
{
	return false;
}



wchar_t* SMUSync::Utf8ToUnicode(const char* utf)
{
	if(!utf || !strlen(utf))
	{
		return NULL;
	}
	int dwUnicodeLen = MultiByteToWideChar(CP_UTF8,0,utf,-1,NULL,0);
	size_t num = dwUnicodeLen*sizeof(wchar_t);
	wchar_t *pwText = (wchar_t*)malloc(num);
	memset(pwText,0,num);
	MultiByteToWideChar(CP_UTF8,0,utf,-1,pwText,dwUnicodeLen);
	return pwText;
}

char* SMUSync::UnicodeToUtf8(const char* unicode)
{
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, NULL, 0, NULL, NULL);
	char *szUtf8 = (char*)malloc(len + 1);
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, szUtf8, len, NULL,NULL);
	return szUtf8;
}

char* SMUSync::UnicodeToAscii(const char* unicode)
{
	int len;
	len = WideCharToMultiByte(CP_OEMCP, 0, (const wchar_t*)unicode, -1, NULL, 0, NULL, NULL);
	char *szUtf8 = (char*)malloc(len + 1);
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_OEMCP, 0, (const wchar_t*)unicode, -1, szUtf8, len, NULL,NULL);
	return szUtf8;
}
