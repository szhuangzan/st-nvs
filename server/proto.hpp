#ifndef __PROTO_H__
#define __PROTO_H__
#include <cstring>
#include <malloc.h>
#include <cassert>
#include "st.h"
typedef unsigned int uint32;
typedef uint32 uint;

//	�ֽ���ת��
#define mcr_ntoh_32(v)		((((v) & 0xFF) << 24) | (((v) & 0xFF00) << 8) | (((v) & 0xFF0000) >> 8) | (((v) & 0xFF000000) >> 24))
#define mcr_ntoh_16(v)		((((v) & 0x00FF) << 8) | (((v) & 0xFF00) >> 8))

//	��ȡ����ԭʼID. (������)
#define command_id(cmdno)	(cmdno & 0xFFFFU)

//	��ȡ������չ��Ϣ. (δ��λ)
#define command_ext(cmdno)	((cmdno & 0x00FF0000U) >> 16)
#define	command_ext_ds_v_frame_p	0x00U
#define command_ext_ds_v_frame_i	0x01U
#define command_ext_ds_a_pcm		0x05U
#define command_ext_ds_a_speex		0x06U

//	��ȡ�����. (δ��λ)
#define command_dir(cmdno)	(cmdno & 0xFF000000U)
#define command_dir_request			0x00000000U
#define command_dir_response		0x80000000U

//	�������
#define make_request_command(v)				((v) | command_dir_request)
#define make_response_command(v)			((v) | command_dir_response)
#define make_request_command_ext(v, e)		((v) | ((e) << 16) | command_dir_request)
#define make_response_command_ext(v, e)		((v) | ((e) << 16) | command_dir_response)

//	�ж��Ƿ���ͬ����
#define command_matched(c1, c2)			(command_id(c1) == command_id(c2))


//------------------------------------------------------------------------------
//	Э��ָ���
//--------------------------------------

//	����ɾ��ָ��������������
static uint32 const hmcmdex_stop_command		=	0;

///	ʵʱ��Ƶ����
//--------------------------------------
static uint32 const hmcmd_open_video			=	0x101u;
static uint32 const hmcmd_recv_video			=	0x102u;
static uint32 const hmcmd_close_video			=	0x103u;
static uint32 const hmcmd_force_iframe			=	0x104u;

///	NVS¼������
//--------------------------------------
static uint32 const hmcmd_open_nvs_replay		=	0x1F1u;
static uint32 const hmcmd_recv_nvs_replay		=	0x1F2u;
static uint32 const hmcmd_close_nvs_replay		=	0x1F3u;

///	ʵʱ��Ƶָ��
//--------------------------------------
static uint32 const hmcmd_open_audio			=	0x201u;
static uint32 const hmcmd_recv_audio			=	0x202u;
static uint32 const hmcmd_close_audio			=	0x203u;

///	�Խ�ָ��
//--------------------------------------
static uint32 const hmcmd_open_talk				=	0x301u;
static uint32 const hmcmd_send_talk				=	0x302u;
static uint32 const hmcmd_close_talk			=	0x303u;

///	�����ϴ�ָ��
//--------------------------------------
static uint32 const hmcmd_open_alarm			=	0x1213;
static uint32 const hmcmd_recv_alarm			=	0x60Cu;
static uint32 const hmcmd_close_alarm			=	0x1214;

static uint32 const hmcmd_set_alarm			=	0x1201u;
static uint32 const hmcmd_reset_alarm		=	0x1202u;

static uint32 const hmcmd_add_area			=	0x1204u;
static uint32 const hmcmd_del_area			=	0x1205u;
static uint32 const hmcmd_change_area		=	0x1206u;

static uint32 const hmcmd_add_sensor		=	0x1207u;
static uint32 const hmcmd_del_sensor		=	0x1208u;
static uint32 const hmcmd_change_sensor		=	0x1209u;

static uint32 const hmcmd_enter_learning		=	0x1210u;
static uint32 const hmcmd_exit_learning			=	0x1211u;
static uint32 const hmcmd_get_paried_sensor		=	0x1212u;

///	Զ��¼��ָ��
//--------------------------------------
static uint32 const hmcmd_remote_open_record	=	0x900u;
static uint32 const hmcmd_remote_close_record	=	0x901u;

///	¼���ѯ
//--------------------------------------
static uint32 const hmcmd_record_query			=	0x701u;
static uint32 const hmcmd_record_delete			=	0x706u;

///	¼��ط�
//--------------------------------------
static uint32 const hmcmd_open_replay			=	0x702u;
static uint32 const hmcmd_recv_replay			=	0x703u;
static uint32 const hmcmd_eof_replay			=	0x704u;
static uint32 const hmcmd_close_replay			=	0x705u;
static uint32 const hmcmd_pause_replay			=	0x707u;
static uint32 const hmcmd_resume_replay			=	0x708u;
static uint32 const hmcmd_step_replay			=	0x709u;
static uint32 const hmcmd_rate_replay			=	0x710u;

///	¼������
//--------------------------------------
static uint32 const hmcmd_open_replay_download	=	0x1001u;
static uint32 const hmcmd_recv_replay_download	=	0x1002u;
static uint32 const hmcmd_close_replay_download	=	0x1003u;

///	ͼƬ��ѯ
//--------------------------------------
static uint32 const hmcmd_capture_image			=	0x801u;
static uint32 const hmcmd_query_image			=	0x802u;
static uint32 const hmcmd_delete_image			=	0x806u;

///	ͼƬ����
//--------------------------------------
static uint32 const hmcmd_open_image_download	=	0x803u;
static uint32 const hmcmd_recv_image_download	=	0x804u;
static uint32 const hmcmd_close_image_download	=	0x805u;

///	��ָ֤��
//--------------------------------------
static uint32 const hmcmd_authencate			=	0x60Du;
static uint32 const hmcmd_online_authencate		=	0x611u;
static uint32 const hmcmd_rev_authencate		=	0x60Au;

///	����ָ��
//--------------------------------------
static uint32 const hmcmd_heartbeat				=	0xA01u;

///	����ָ��
//--------------------------------------
//define_command(hmcmd_online_info,		0xB01u,	0);
static uint32 const hmcmd_population_flow		=	0x60Bu;
static uint32 const hmcmd_restart_device		=	0x60Eu;
static uint32 const hmcmd_close_connection		=	0x60Fu;
static uint32 const hmcmd_sync_time				=	0x610u;
static uint32 const hmcmd_connection_inner_auth	=	0x611u;
static uint32 const hmcmd_nvs_redirect			=	0x612u;
static uint32 const hmcmd_ptz_ctrl				=	0x401u;

static uint32 const hmcmd_set_param				=	0x501u;
static uint32 const hmcmd_get_param				=	0x502u;

static uint32 const hmcmd_set_config			=	0x503u;
static uint32 const hmcmd_get_config			=	0x504u;

static uint32 const hmcmd_reset_device			=	0xB01u;

///	״̬�ı�֪ͨ
static uint32 const hmcmd_states_change_open	=	0x505u;
static uint32 const hmcmd_states_change_report	=	0x506u;

//	������
static uint32 const hmcmd_defence_enable		=	0x1201;
static uint32 const hmcmd_defence_disable		=	0x1202;
static uint32 const hmcmd_defence_get_info		=	0x1203;

//	��������
static uint32 const hmcmd_get_sysinfo			=	0x1109u;
static uint32 const hmcmd_restore_default		=	0x1108u;
static uint32 const hmcmd_open_update			=	0x1104u;
static uint32 const hmcmd_update_data			=	0x1105u;
static uint32 const hmcmd_close_update			=	0x1106u;
static uint32 const hmcmd_update_result			=	0x1107u;
static uint32 const hmcmd_sd_format				=	0x1103u;
static uint32 const hmcmd_upnp_test				=	0x1102u;
static uint32 const hmcmd_query_wifi			=	0x1101u;
static uint32 const hmcmd_device_lock 			=	0x1301u;
static uint32 const hmcmd_privacy_protect 		=	0x1303u;



#define MAX_FRAME_BUF 40000
typedef unsigned int uint32;

typedef struct net_port_header_t
{
	uint32	cmdno;		//	�����.
	uint32	bodylen;	//	��Ϣ���ȣ��������ʵ�ʳ��ȣ���������ͷ����
	uint32	errcode;	//	������.
	uint32	serialid;	//	���к�.
}net_port_header_t;


#include <cstdio>
typedef struct net_port_msg_t
{
	net_port_header_t hdr;
	char*	body;
	st_cond_t cond;
	uint32	ref;
	net_port_msg_t()
		:ref(1)
		,hdr()
		,body(0)
		,cond(0)
	{
		
	}
	uint32 add()
	{
		ref++;
		return ref;
	}
	void release()
	{
		assert(ref>=1);
		
		ref--;
		if(ref == 0)
		{
			delete this;
		}
		return ;
	}
	~net_port_msg_t()
	{
		//if(cond) assert(st_cond_destroy(cond) !=-1);
		cond = 0;
		if(body) free(body);
		body = 0;
	}
}net_port_msg_t;

//--------------------------------------
struct standard_protocol_t
{
public:	//	�ֽ���ת��. ��׼����²���Ҫ.
	void transfer_in(net_port_header_t & hdr)	{}
	void transfer_out(net_port_header_t & hdr)	{}
public:	//	Э��ͷ����, Ĭ��Ϊͷ����С.
	uint header_size() { return sizeof(net_port_header_t); }
public:	//	�ж�ָ���Ƿ���Զ�̷���������. (�����Ǳ�������Ļ�Ӧ)
	bool is_request_command(uint32 cmdno)
	{
		return cmdno & 0x80000000U;
	}
public:	//	�ж�ָ���Ƿ��ǳ�Чָ��. (���к�������������ж�λ�Ӧ.)
	bool is_longlive_command(uint32 cmdno)	{ return cmdno & 0x40000000U; }
public:	//	�жϻ�Ӧָ���Ƿ�ƥ��
	static bool match_command(uint32 serial, uint32 cmd, uint32 sserial, uint32 scmd)
	{ return (serial == sserial); }
public:
	static bool is_bad_header(net_port_header_t & hdr) { return false; }


};


static void transfer_in(net_port_header_t&  hdr)
{
	hdr.cmdno	= mcr_ntoh_32(hdr.cmdno);
	hdr.bodylen	= mcr_ntoh_32(hdr.bodylen);
	hdr.errcode	= mcr_ntoh_32(hdr.errcode);
	hdr.serialid= mcr_ntoh_32(hdr.serialid);
}
static void transfer_out(net_port_header_t& hdr)
{
	transfer_in(hdr);
}

class  server_base_t
{
public:
	server_base_t()
		:_ref(1)
	{
		_notify_cond = st_cond_new();
	}
	uint32 add()
	{
		return _ref++;
	}

	void release()
	{
		assert(_ref);
		_ref--;
		if(_ref == 0)
		{
			delete this;
		}
		
	}

	virtual ~server_base_t()
	{
		
	}
protected:
	void wait()
	{
		if(_notify_cond) st_cond_wait(_notify_cond);
	}

	void notify()
	{
		if(_notify_cond) st_cond_signal(_notify_cond);
	}

private:
	uint32				 _ref;
private:
	st_cond_t			 _notify_cond;
};


typedef struct socket_fd
{
	st_netfd_t	_fd;
	bool		_net_valid;
	socket_fd()
		:_net_valid(false)
	{
		
	}
	 
	void init(st_netfd_t fd)
	{
		assert(fd);
		_fd = fd;
		_net_valid = true;

	}
	~socket_fd()
	{
		assert(_fd);
		assert(st_netfd_close(_fd)  != -1);
	}

	bool is_vaild()
	{
		return _net_valid;
	}

	void uninit()
	{
		assert(_net_valid);
		_net_valid = false;
	}

	st_netfd_t get_fd()
	{
		return  _fd;
	}

}socket_fd_t;

#endif