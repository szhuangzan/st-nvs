#ifndef __PROTO_H__
#define __PROTO_H__
//	字节序转换
#define mcr_ntoh_32(v)		((((v) & 0xFF) << 24) | (((v) & 0xFF00) << 8) | (((v) & 0xFF0000) >> 8) | (((v) & 0xFF000000) >> 24))
#define mcr_ntoh_16(v)		((((v) & 0x00FF) << 8) | (((v) & 0xFF00) >> 8))

typedef unsigned int uint32;

typedef struct net_port_header_t
{
	uint32	cmdno;		//	命令号.
	uint32	bodylen;	//	消息长度（数据体的实际长度，不包含本头部）
	uint32	errcode;	//	错误码.
	uint32	serialid;	//	序列号.
}net_port_header_t;

static void transfer_in(net_port_header_t*  hdr)
{
	hdr->cmdno	= mcr_ntoh_32(hdr->cmdno);
	hdr->bodylen	= mcr_ntoh_32(hdr->bodylen);
	hdr->errcode	= mcr_ntoh_32(hdr->errcode);
	hdr->serialid= mcr_ntoh_32(hdr->serialid);
}
static void transfer_out(net_port_header_t* hdr)
{
	transfer_in(hdr);
}

#endif