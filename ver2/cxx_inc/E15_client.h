#ifndef __E15_client_H
#define __E15_client_H

#include "E15_object.h"
#include "E15_socket.h"


typedef struct E15_ClientInfo
{
	E15_Id        id;
	const char * name;
	const char * pwd;
	const char * role;

	E15_Id        user_id;
	void        * user_obj; //用户绑定的对象
}E15_ClientInfo;

#pragma pack(1)
typedef struct E15_ClientMsg
{
/*  type
//中文Bit7       |    bit6            |  bit5	       |  bit4	bit3	bit2	bit1	bit0 |
//响应的标志     |   异步通知标志     | 系统接口标志    |   当bit5=1时：系统接口编号 0x1f     |
0: 请求          |   0：普通请求      | 0:业务接口      |   当bit5=0时：用户接口类型          |
1：响应"	         |   1：异步事件      | 1:系统接口      |
*/
    unsigned char    type;
	E15_Id  receiver;
	E15_Id  sender;
	unsigned short cmd;
	unsigned int seq;
	int status;
}E15_ClientMsg;
#pragma pack()

class E15_cxx_object_API E15_Client : public E15_Object
{
public:
    E15_Client();
    virtual ~E15_Client();

    void Start(E15_Socket * sock);
    void Stop();

    int Login(E15_ClientInfo * info,const char * json,const char * addr,int port);
    void Logout(E15_Id *userid);

    int Request(E15_Id * id,E15_ClientMsg * cmd,const char * json,int json_len);
    int Notify(E15_Id * id,E15_ClientMsg * cmd,const char * json,int json_len,int safe_flag = 0);
    void Response(E15_Id * id,E15_ClientMsg * cmd,const char * json,int json_len);

public:
	virtual void OnLoginOk(E15_ClientInfo * user,E15_String *& json) = 0;
	virtual void OnRequest(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json) = 0;
	virtual void OnResponse(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json) = 0;

	virtual void OnNotify(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json);
	virtual int OnLoginFailed(E15_ClientInfo * user,int status,const char * errmsg);
	virtual int OnLogout(E15_ClientInfo * user);

};


#endif
