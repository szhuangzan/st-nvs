#include <malloc.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <string>
#include <Windows.h>

#pragma  comment(lib, "libeay32.lib")

char * Base64Encode(const char * input, int length)
{
	BIO * bmem = NULL;
	BIO * b64 = NULL;
	BUF_MEM * bptr = NULL;

	b64 = BIO_new(BIO_f_base64());

	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	char * buff = (char *)malloc(bptr->length + 1);
	memcpy(buff, bptr->data, bptr->length);
	buff[bptr->length] = 0;

	BIO_free_all(b64);

	return buff;
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

int main(int argc, char* argv[])
{
	FILE* src_cfg = NULL;
	FILE* dst_cfg = NULL;
	char path[256] = {};

	GetModuleFileName(NULL, path, 256);
	std::string strPath =path;
	int pos = strPath.find_last_of("\\");
	strPath = strPath.substr(0,pos+1);
	strPath.append("server.cfg");

	src_cfg = fopen(strPath.c_str(), "r+");
	if(!src_cfg)
	{
		printf("当前目录找不到正确的配置文件server.cfg!\n");
		return NULL;
	}

	char* src_encry = NULL;
	

	{
		fseek(src_cfg, 0, SEEK_END);
		int file_len = ftell(src_cfg);
		fseek(src_cfg, 0, SEEK_SET);
		char* src_data = (char*)calloc(1, file_len);
		int read_len = fread(src_data, 1, file_len, src_cfg);
		if(read_len != file_len)
		{
			free(src_data);
			fclose(src_cfg);
			return 0;
		}
		src_encry = Base64Encode(src_data, read_len);
		free(src_data);
		fclose(src_cfg);
	}

	int src_necry_len = strlen(src_encry);

	{
		dst_cfg = fopen("server.xcfg", "w+");
		if(!dst_cfg)
		{
			printf("无法创建加密配置文件server.xcfg!\n");
			return NULL;
		}

		int write_len = fwrite(src_encry, 1, src_necry_len, dst_cfg);
		if(write_len != src_necry_len)
		{
			printf("写配置文件server.xcfg异常!\n");
			return NULL;
		}
		free(src_encry);
		fclose(dst_cfg);

		printf(" ********************************************************************\n");
		printf("    成功生成配置文件server.xcfg   \n");
		printf("    为了安全请删除server.cfg, 服务器程序只需要server.xcfg ! \n");
		printf(" ********************************************************************\n");
	}


	return 0;
}