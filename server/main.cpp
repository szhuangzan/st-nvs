#include "st.h"
#include "neterr.h"
#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <errno.h>
#include "server.hpp"
static st_thread_t maintid;
static int count=0;

#pragma comment(lib, "st.lib")
#pragma comment(lib, "WS2_32.lib")

#include <windows.h>
extern int server(int argc, char *argv[]);



#include<string>
#include<windows.h>
#include<vector>
using namespace std;

//utf8 转 Unicode


std::wstring Utf82Unicode(const std::string& utf8string)
{
	int widesize = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0);
	if (widesize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (widesize == 0)
	{
		throw std::exception("Error in conversion.");
	}

	std::vector<wchar_t> resultstring(widesize);

	int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &resultstring[0], widesize);

	if (convresult != widesize)
	{
		throw std::exception("La falla!");
	}

	return std::wstring(&resultstring[0]);
}


//unicode 转为 ascii


std::string WideByte2Acsi(std::wstring& wstrcode)
{
	int asciisize = ::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, NULL, 0, NULL, NULL);
	if (asciisize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (asciisize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<char> resultstring(asciisize);
	int convresult =::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, &resultstring[0], asciisize, NULL, NULL);

	if (convresult != asciisize)
	{
		throw std::exception("La falla!");
	}

	return std::string(&resultstring[0]);
}





//utf-8 转 ascii


std::string UTF_82ASCII(std::string& strUtf8Code)
{
	std::string strRet("");
	//先把 utf8 转为 unicode
	std::wstring wstr = Utf82Unicode(strUtf8Code);
	//最后把 unicode 转为 ascii
	strRet = WideByte2Acsi(wstr);
	return strRet;
}


///////////////////////////////////////////////////////////////////////


//ascii 转 Unicode


std::wstring Acsi2WideByte(std::string& strascii)
{
	int widesize = MultiByteToWideChar (CP_ACP, 0, (char*)strascii.c_str(), -1, NULL, 0);
	if (widesize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (widesize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = MultiByteToWideChar (CP_ACP, 0, (char*)strascii.c_str(), -1, &resultstring[0], widesize);


	if (convresult != widesize)
	{
		throw std::exception("La falla!");
	}

	return std::wstring(&resultstring[0]);
}


//Unicode 转 Utf8


std::string Unicode2Utf8(const std::wstring& widestring)
{
	int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8size == 0)
	{
		throw std::exception("Error in conversion.");
	}

	std::vector<char> resultstring(utf8size);

	int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);

	if (convresult != utf8size)
	{
		throw std::exception("La falla!");
	}

	return std::string(&resultstring[0]);
}


//ascii 转 Utf8


std::string ASCII2UTF_8(std::string& strAsciiCode)
{
	std::string strRet("");
	//先把 ascii 转为 unicode
	std::wstring wstr = Acsi2WideByte(strAsciiCode);
	//最后把 unicode 转为 utf8
	strRet = Unicode2Utf8(wstr);
	return strRet;
}
void* handle_connect(void*arg)
{
	char cc[1024] = "Provider=SQLOLEDB;Server=BGONG-B75DC97C7;Database=TestAlarm;User ID=sa;Password=147258abc;Data Source=192.168.10.186";
	int errcode = 0;
	char buf[128] = {};
	st_dbfd_t dbfd = st_db_connect(cc, &errcode, buf, 128);
	printf("ok\n");

	/*	errcode = st_db_query(dbfd, "select UserName from HM_User;", buf, 128);
		printf("ok\n");
		if(errcode==0)
		{
			printf("ok\n");
			std::vector<std::wstring> ss;
			errcode = st_db_fetch(dbfd, "UserName",ss, buf,128);
			if(errcode)printf("errr\n");
			for(int i=0;i<ss.size();i++)
			{
				printf("%s\n",UnicodeToAscii(ss[i].c_str()));
			}
		}*/

		printf("ok\n");

{
	
	//if(st_db_exec(dbfd, "insert into [HM_User](UserID,AreaID,SerialNumber) values('11111','100001','11111')")!=0)
	//{
	//	printf("caozuo\n");
	//}
	st_db_close(dbfd);
}
	return NULL;
}
int main(int argc, char *argv[])
{

	//st_init();
	//st_thread_create(handle_connect, 0,0,0);
	//st_sleep(10000);
	/*st_netfd_t srvfd = NULL;
	int n = 0;
	struct sockaddr_in serv_addr;
	struct hostent *hp = 0;
	unsigned short port = 0;



	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(0);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	srvfd = st_netfd_listen(serv_addr);
	st_thread_create(handle_connect, 0,0,0);
	st_sleep(10000);*/
	ServerHandler srv;

	srv.run();

	return(0);
}

