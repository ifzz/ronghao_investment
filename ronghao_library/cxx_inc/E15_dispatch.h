#ifndef __E15_dispatch_H
#define __E15_dispatch_H

#include "E15_object.h"
#include "E15_thread.h"
enum E15_Msg_Type
{
	E15_Msg_Type_Send,
	E15_Msg_Type_Push,
	E15_Msg_Type_Post
};

typedef struct E15_Msg
{
	E15_Msg_Type    type;
	unsigned long 	msg;
	long	      	submsg;

	E15_Id	      	*sender;
	E15_Id			*receiver;
	E15_Object *    params;
}E15_Msg;


class E15_cxx_object_API E15_DispatchHandler : public E15_Object
{
public:
    E15_DispatchHandler();
    virtual ~E15_DispatchHandler();

    static void Return(const E15_Msg * msg,long ret);//msg是在OnMessage中回调事件，复制无效。

    virtual long OnMessage(E15_Msg * msg) = 0;
    virtual void Notify();
};

class E15_cxx_object_API E15_Dispatch : public E15_Object
{
	//设置事件处理器的对象，处理函数，处理器运行所在的线程ID
public:
    E15_Dispatch();
    virtual ~E15_Dispatch();

    //使用内部创建线程来分发事件,name_fmt,...用来标示线程名字，方便调试
	void Start(E15_DispatchHandler * handler_obj,const char * name_fmt,...);
	//使用外部线程自行获取事件处理
	void Start(E15_DispatchHandler * handler_obj,unsigned long thd_id );//参数 thd_id为了匹配是否在同一线程中运行

	void Stop(); //

	//同步调用，等待返回,E15_Object 调用者不可再使用
	long Send(E15_Id * sender,E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0 );
	//异步调用，立即返回，对于事件太多，可能被抛弃,E15_Object 调用者不可再使用
	long Post(const E15_Id * sender,const E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0);
	//异步调用，立即返回，并且不可抛弃,E15_Object 调用者不可再使用
	long Push(const E15_Id * sender,const E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0);

	//////
	long SendFirst(E15_Id * sender,E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0 );
	//异步调用，立即返回，对于事件太多，可能被抛弃,E15_Object 调用者不可再使用
	long PostFirst(const E15_Id * sender,const E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0);
	//异步调用，立即返回，并且不可抛弃,E15_Object 调用者不可再使用
	long PushFirst(const E15_Id * sender,const E15_Id * receiver,unsigned long msgid,long sub_msgid,E15_Object * params = 0);

	int TryMsg(unsigned int wait_ms);//当Start指定了线程时，就再线程中用这个来逐一处理消息
};



#endif

