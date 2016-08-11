#ifndef __E15_udcp_H
#define __E15_udcp_H

#include "E15_object.h"
#include "E15_string.h"
#include "E15_socket.h"


typedef struct E15_UdcpInfo
{
	const E15_Id * id; //连接的ID标示，在udcp内唯一
	const char * addr;//该id对应连接的对端地址
	const int port;//该id对应连接的对端端口
	const char	byte_order;
	const unsigned char 	master; //系统提示该id对应连接是属于主动发起（master & 0x7f == 1),还是被动接受(master & 0x7f == 0)
}E15_UdcpInfo;

class E15_Udcp;

class E15_cxx_object_API E15_UdcpItem : public E15_Object
{
public:
    E15_UdcpItem();
    virtual ~E15_UdcpItem();

	virtual void OnConnectOk(E15_Udcp * udcp,E15_UdcpInfo * info); //连接成功，master_flag = 0表示是被动连接，其余为主动连接
	//返回值 < 0 ，不再重连，= 0,立即重连， > 0 指定的时间后重连
	virtual int OnConnectFailed(E15_Udcp * udcp,E15_UdcpInfo * info);//连接失败，可为0
	virtual int OnDisConnect(E15_Udcp * udcp,E15_UdcpInfo * info); //已经建立的连接断开

	virtual void OnData(E15_Udcp * udcp,E15_UdcpInfo * info,E15_String *& data,unsigned int delay) = 0;//得到远端数据
	virtual void OnPost(E15_Udcp * udcp,E15_UdcpInfo * info,E15_String *& data,unsigned int delay);//得到远端数据Post方式数据，可为0
	virtual void OnSendError(E15_Udcp * udcp,E15_UdcpInfo * info,E15_String *& data,unsigned int delay);//发送数据失败，超时未完成,可为0
    virtual void OnSendQueueStatus(E15_Udcp * udcp,E15_UdcpInfo * info,int flag);
public:
    const E15_Id  udcp_id ;
};

class E15_cxx_object_API E15_Udcp : public E15_Object
{
public:
    E15_Udcp();
    virtual ~E15_Udcp();

    //被动连接的处理
    virtual E15_UdcpItem * OnConnectRequest(E15_UdcpInfo * info);
	//指定连接参数种类，一共有4种,各自的队列和定时器有差异
/*
	schema =
	0  : 优质网络，对应低延迟，高吞吐，快速检测网络状态 （适应通信数据大的内网）
	1  : 优质网络，对应低延迟，低吞吐，快速检测网络状态 （适应于通信数据少的内网）,可以少消耗缓存
	2  : 低速不稳定网络，对应高延迟，低吞吐，低速检测网络状态 （移动网络)，该情况下，网络ip地址端口都可能发生变化，需要检测应对
*/
	void Init(E15_Socket * sock,int schema);

	//clientonly，仅仅作为主动端，不接受连接请求
	int Start(int clientonly,const char * local_addr,int local_port);
	void Stop();

	//可考虑在这里作数据绑定
	int Connect(const char * remote_addr,int remote_port,E15_UdcpItem * item);
	int DisConnect(const E15_Id * id);

	void Post(const  E15_Id * id,const char * data,int len);
	int Send(const  E15_Id * id,const char * data,int len,int safe_flag = 0);

	void PostString(const  E15_Id * id,E15_String * s);
	int SendString(const  E15_Id * id,E15_String * s,int safe_flag = 0);
	//更改连接的目的地址
	int ChangeDestAddr(E15_Id * id,const char * addr,int port);
};

#endif
