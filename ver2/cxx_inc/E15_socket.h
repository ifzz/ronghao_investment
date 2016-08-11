#ifndef __E15_socket_H
#define __E15_socket_H

#include <stdarg.h>

#include "E15_object.h"

#include "E15_sockaddr.h"
#include "E15_string.h"


typedef enum E15_Socket_Type
{
	E15_Socket_unknown = 0,
	E15_Socket_tcp_client,
	E15_Socket_tcp_server,
	E15_Socket_udp,
}E15_Socket_Type;

typedef struct E15_Socket_Info
{
	E15_Socket_Type type;
	E15_Id			id;
	int				fd;
	E15_SockAddr * local;
	E15_SockAddr * remote;

	E15_Id 		 user_id;

}E15_Socket_Info;


class E15_Socket;

class E15_cxx_object_API E15_SocketCallback
{
public:
    virtual int OnDataOk(E15_Socket *obj,E15_Socket_Info * info,E15_String *& data) = 0;
};

class  E15_cxx_object_API E15_SocketUdpCallback : public E15_SocketCallback
{
public:
	virtual int OnDataOk(E15_Socket *obj,E15_Socket_Info * info,E15_String *& data) = 0;
};

class  E15_cxx_object_API E15_SocketTcpCallback : public E15_SocketCallback
{
public:
	virtual int OnDataOk(E15_Socket *obj,E15_Socket_Info * info,E15_String *& data) = 0;
	virtual void OnConnectOk(E15_Socket *obj,E15_Socket_Info * info,int type) = 0;
	//注意：对于某些严重错误（如：地址错误，bind本地地址错误等情况，系统不再重连）
	virtual int OnConnectFaild(E15_Socket *obj,E15_Socket_Info * info,int err) = 0; //返回值< 0 不再重连，== 0 立即重连， > 0, 过指定的时间 ms重连
	//主动连接或者被动连接,type = 1,可重连,0被动连接，不可重连
	virtual int OnDisConnect(E15_Socket *obj,E15_Socket_Info * info,int type) = 0; //返回值< 0 不再重连，== 0 立即重连， > 0, 过指定的时间 ms重连
	//发送队列中的数据已经完成，没有待发送数据了
	virtual void OnSendQueueEmpty(E15_Socket *obj,E15_Socket_Info * info);
	virtual void OnSendQueueFull(E15_Socket *obj,E15_Socket_Info * info,int flag);
};

class  E15_cxx_object_API E15_SocketListenCallback : public E15_SocketCallback
{
private:
    virtual int OnDataOk(E15_Socket *obj,E15_Socket_Info * info,E15_String ** data)
    {
        return 0;
    }

public:
	//当有客户端连接时
	virtual void OnListenResult(E15_Socket *obj,E15_Socket_Info * info,int success);
	virtual E15_SocketTcpCallback *  OnAccept(E15_Socket *obj,E15_Socket_Info * server,E15_Socket_Info * client) = 0 ;//返回0表示不接受该连接，非0则接受
	//当某个连接被关闭时
	virtual int OnChildClose(E15_Socket *obj,E15_Socket_Info * info,E15_Socket_Info * client) ;
};


class  E15_cxx_object_API E15_Socket : public E15_Object
{
public :
    E15_Socket();
    virtual ~E15_Socket();

	void Start(); //
	void Stop();

	int Open(E15_Socket_Info *info,E15_SocketUdpCallback * cb);
	int Listen(E15_Socket_Info *info,E15_SocketListenCallback *cb);
	int Connect(E15_Socket_Info *info,E15_SocketTcpCallback * cb);
	void Close(E15_Id * id);

	//格式化发送数据用的函数，返回值 <= 0 失败,>0 成功
	int Printf(E15_Id * id,const char * fmt,...);
	int PrintfV(E15_Id * id,const char * fmt,va_list ap);

	//发送数据用的函数，返回值 <= 0 失败,>0 成功
	int SendData(E15_Id * id,const char * data,int len);
	int SendString(E15_Id * id,E15_String **data);

};


#endif
