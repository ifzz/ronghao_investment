#ifndef __E15_server_H
#define __E15_server_H

#include "E15_object.h"
#include "E15_string.h"

#include "E15_socket.h"

typedef struct E15_ServerInfo
{
	E15_Id id;

	unsigned int N;
	const char * name;
	const char * role;

	char 		is_addr_change;
	const char * ip;     //把ip地址信息提交给上层做参考
	int          port;

	E15_Id 				user_id;
	unsigned int	   	user_type;
	void 			* 	user_obj; //外部绑定
	char byte_order;
}E15_ServerInfo;

#pragma pack(1)

typedef struct E15_ServerRoute
{
	unsigned short index; //路径编号
	E15_Id   * net_list;
}E15_ServerRoute;

typedef struct E15_ServerCmd
{
	unsigned char type;
	E15_Id  receiver;
	E15_Id  sender;
	unsigned short cmd;
	unsigned int seq;
	int status;
}E15_ServerCmd;

#pragma pack()


class E15_cxx_object_API E15_Server : public E15_Object
{
public:
    E15_Server();
    virtual ~E15_Server();

    int Start(E15_Socket * sock,const char * inifile);
    void Stop();

    void Close(E15_Id *id);

    int Request(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const E15_String * json);
    int Notify(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const E15_String * json,int safe_flag = 0);
    void Response(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const E15_String * json);

    int Request(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const char * data,int len);
    int Notify(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const char * data,int len,int safe_flag = 0);
    void Response(E15_Id * id,E15_ServerRoute * rt,E15_ServerCmd * cmd,const char * data,int len);

public:
	/*
	OnOpen return
	< 0 ;//不需要的连接
	= 0 ;//连接成功，不需要返回json数据
	> 0 ;//json数据已经更改，需要返回给远端
	*/

	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json) = 0;
	virtual int OnClose(E15_ServerInfo * info) = 0;
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) = 0;
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) = 0;

	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
};


#endif
