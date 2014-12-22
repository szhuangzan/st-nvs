#include "ParseCfg.h"
#include <malloc.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <malloc.h>

static ParseCfg* _ParseCfg_Inst = 0;

ParseCfg* GetCfgInst()
{
	if(!_ParseCfg_Inst)
	{
		_ParseCfg_Inst = new ParseCfg;
	}
	return _ParseCfg_Inst;
}

static int base64_decode(char *str,int str_len,char *decode,int decode_buffer_len)
{
	BIO * b64 = NULL;
	BIO * bmem = NULL;
	b64 = BIO_new(BIO_f_base64());
	
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem = BIO_new_mem_buf(str, str_len);
	bmem = BIO_push(b64, bmem);
	BIO_read(bmem, decode, decode_buffer_len);

	BIO_free_all(bmem);
	return 0;
}

ParseCfg::ParseCfg()
{

}

ParseCfg::~ParseCfg()
{

}

bool ParseCfg::ReadCfg(const char* cfg)
{
	FILE* file = NULL;
	char decry_buf[4096] = {};
	file = fopen(cfg, "r+");
	if(!file)
	{
		return false;
	}

	char* encry_data = 0;
	{
		int file_len = 0;
		
		fseek(file, 0, SEEK_END);
		file_len = ftell(file);
		fseek(file, 0, SEEK_SET);
		encry_data = (char*)calloc(1, file_len+1);
		int read_len = fread(encry_data, 1, file_len, file);
		if(read_len != file_len)
		{
			return false;
		}
		base64_decode(encry_data, file_len, decry_buf, sizeof(decry_buf));
	}

	{
		DMXml xml;
		xml.Decode(decry_buf);

		{
			char* tmp = xml.GetRoot()->FindElement("DB")->FindElement("Server")->GetValueText();
			if(!tmp) return false;
			_db_connect_info._server = tmp;
			free(tmp);
		}
	
		{
			char* tmp = xml.GetRoot()->FindElement("DB")->FindElement("Database")->GetValueText();
			if(!tmp) return false;
			_db_connect_info._database_name = tmp;
			free(tmp);
		}
		{
			char* tmp = xml.GetRoot()->FindElement("DB")->FindElement("UserID")->GetValueText();
			if(!tmp) return false;
			_db_connect_info._dbms_user_name = tmp;
			free(tmp);
		}

		{
			char* tmp = xml.GetRoot()->FindElement("DB")->FindElement("Password")->GetValueText();
			if(!tmp) return false;
			_db_connect_info._dbms_user_pwd = tmp;
			free(tmp);
		}

		{
			char* tmp = xml.GetRoot()->FindElement("DB")->FindElement("DataSource")->GetValueText();
			if(!tmp) return false;
			_db_connect_info._server_ip = tmp;
			free(tmp);
		}
		{
			int tmp = xml.GetRoot()->FindElement("DB")->FindElement("Port")->GetValueInt();
			if(!tmp) return false;
			_db_connect_info._server_port = tmp;
		}

		{
			char* tmp = xml.GetRoot()->FindElement("SyncServer")->FindElement("IP")->GetValueText();
			if(!tmp) return false;
			_server_ip = tmp;
			free(tmp);
		}

		{
			int tmp = xml.GetRoot()->FindElement("SyncServer")->FindElement("Port")->GetValueInt();
			if(!tmp) return false;
			_server_port = tmp;
		}
#if 0
		{
			char* tmp = xml.GetRoot()->FindElement("Plat")->FindElement("Url")->GetValueText();
			if(!tmp) return false;

			{
				hostent* host = gethostbyname(tmp);
				char buf[20] ={};
				sprintf_s(buf, 20, "%d.%d.%d.%d",
					host->h_addr_list[0][0]&0xff,
					host->h_addr_list[0][1]&0xff,
					host->h_addr_list[0][2]&0xff,
					host->h_addr_list[0][3]&0xff);
				_plat_login_info.PlatUrl = buf;
			}
			free(tmp);
		}


		{
			int tmp = xml.GetRoot()->FindElement("Plat")->FindElement("Port")->GetValueInt();
			if(!tmp) return false;
			_plat_login_info.PlatPort = tmp;
		}

		{
			char* tmp = xml.GetRoot()->FindElement("Plat")->FindElement("UserName")->GetValueText();
			if(!tmp) return false;
			_plat_login_info.PlatUserName = tmp;
			free(tmp);
		}

		{
			char* tmp = xml.GetRoot()->FindElement("Plat")->FindElement("Password")->GetValueText();
			if(!tmp) return false;
			_plat_login_info.PlatPwd = tmp;
			free(tmp);
		}
#endif
	}

	return true;
}

bool ParseCfg::GetServerInfo(std::string& server_ip, unsigned short& server_port)
{
	server_ip = _server_ip;
	server_port = _server_port;
	
	return true;
}

bool ParseCfg::GetDBInfo(DBConnectInfo_t& db)
{
	db._database_name = _db_connect_info._database_name;
	db._dbms_user_name = _db_connect_info._dbms_user_name;
	db._dbms_user_pwd = _db_connect_info._dbms_user_pwd;
	db._server = _db_connect_info._server;
	db._server_ip = _db_connect_info._server_ip;
	db._server_port = _db_connect_info._server_port;
	return true;
}

PlatLoginInfo&  ParseCfg::GetPlatLoginInfo()
{
	return _plat_login_info;
}