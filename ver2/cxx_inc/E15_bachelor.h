#ifndef __E15_bachelor_H
#define __E15_bachelor_H

#include "E15_object.h"
#include "E15_string.h"
#include "E15_socket.h"


class E15_cxx_object_API E15_BachelorInfo
{
public:
	const char * name;
	unsigned int N; //系统内的编号,1~65536 ,如果该值为0，表示非系统节点
	const char * role;
	const char * group;
	short	     balance;

	const char * ip;
	unsigned short port;

	char			byte_order;
};

class E15_Bachelor;

class E15_cxx_object_API  E15_BachelorMate : public E15_Object
{
public:
    E15_BachelorMate();
    virtual ~E15_BachelorMate();

	virtual void Online(E15_Bachelor * bachelor,E15_Id * id,E15_BachelorInfo * info ) = 0;
	virtual int Offline(E15_Bachelor * bachelor,E15_Id * id,E15_BachelorInfo * info ) = 0;

	virtual void OnData(E15_Bachelor * bachelor,E15_Id * id,E15_BachelorInfo * info ,E15_String *&data,int delay) = 0;
	virtual void OnPost(E15_Bachelor * bachelor,E15_Id * id,E15_BachelorInfo * info ,E15_String *&data,int delay) = 0;

	virtual void OnSendQueueStatus(E15_Bachelor * bachelor,E15_Id * id,E15_BachelorInfo * info ,int flag);

public:
	int Send(const char * data,int len,int safe_flag = 0);
	int SendString(E15_String * data,int safe_flag = 0);

	void Post(const char * data,int len);
	void PostString(E15_String * data);

	void DisConnect();

protected:
    const E15_Id m_id;
    E15_Bachelor * m_bachelor;

    friend class E15_Bachelor;
    friend void OnRequestConnect(E15_BachelorMate * mate,E15_Bachelor * bachelor,E15_Id * id );
};

class E15_cxx_object_API  E15_Bachelor : public E15_Object
{
public:
    E15_Bachelor();
    virtual ~E15_Bachelor();

	virtual void OnRegisterOk(const E15_BachelorInfo *); //向domain注册后，会得到注册成功，注册失败等通知
	virtual void OnRegisterFailed();//向domain注册后，会得到注册成功，注册失败等通知

	virtual E15_BachelorMate * OnRequest(const E15_BachelorInfo *) = 0; //新的连接，需要对应的处理


	void Init(E15_Socket * sock,int local_port = 0);//local_port == 0 表示系统分配端口
	void Register(const char * my_name,const char * addr,int port,const char * domain_name = "default" ); //domain的地址

	void Stop();

	int Connect(const char * name,const char * addr,int port,E15_BachelorMate * mate);//临时连接，不经过domain服务器,可以建立多个连接，使用多个名字
	int DisConnect(const E15_Id * id);//删除临时链接

	E15_BachelorInfo * SelfInfo();
};

#endif
