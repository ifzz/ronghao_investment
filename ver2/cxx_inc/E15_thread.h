#ifndef _E15_thread_H
#define _E15_thread_H

#include "E15_object.h"

#define E15_infinite_wait -1

class  E15_cxx_object_API E15_Lock : public E15_Object
{
public:
    E15_Lock();
    virtual ~E15_Lock();

	void Lock( );	//加锁
	void Unlock( );	//解锁
};

class  E15_cxx_object_API E15_AutoLock : public E15_Object
{
public:
	E15_AutoLock( E15_Lock * lock);
    virtual ~E15_AutoLock();
private:
    E15_Lock * plock;
};

//////////////////////////////////////////////////////////////////////////

class  E15_cxx_object_API E15_Event : public E15_Object
{
public:
    E15_Event(int bManualReset =1,int bInitialState = 0);
    virtual ~E15_Event();

	void Set();
	void Reset();
	//改变下策略，wait > 0 成功，= 0 超时
	int Wait(unsigned long dwMilliseconds = E15_infinite_wait); //等待事件触发，最大等待dwMilliseconds毫秒，如果 dwMilliseconds == -1,无限等待
};

///////////////////////////////////////////////////////////////////

class  E15_cxx_object_API  E15_Mutex : public E15_Object
{
public:
    E15_Mutex(int bInitialOwner = 0,const char* mutexName = 0 );
    virtual ~E15_Mutex();

	int Lock(unsigned long delay = -1);//加锁，获取控制，delay表示等待时间，delay = -1表示无限等待
	int Unlock();
	int IsLocked();
};

class  E15_cxx_object_API E15_Semaphore : public E15_Object
{
public:
    E15_Semaphore(unsigned int initsps = 0 ,unsigned int maxsps = 100,const char *spName = 0 );
    virtual ~E15_Semaphore();

	int Wait(unsigned long delay = -1);//加锁，获取控制，delay表示等待时间，delay = -1表示无限等待
	int Post();
	int PostN(unsigned int sps); //sps 信号数量，默认为1
};


//////////////////////////////////////////////

#ifdef WIN32
        typedef void * E15_ThreadHandle;
#else
        typedef unsigned long int E15_ThreadHandle;
#endif

typedef unsigned long (*E15_ThreadFunc)(void*);

class  E15_cxx_object_API  E15_Thread : public E15_Object
{
public:
    E15_Thread(const char * name);
    virtual ~E15_Thread();

	//初始化部分函数，需要在resume之前调用
	void Init(E15_ThreadFunc aFunc,void * aParam);//指定运行函数，参数
	void InitWithStack(E15_ThreadFunc aFunc,void * aParam,void * aStack,unsigned int aStackSize);//暂时未实现

	unsigned long GetCurrentId();//获得当前线程的ID
	unsigned long GetId(); //获得线程ID
	E15_ThreadHandle GetHandle(); //获得线程句柄（windows下有句柄）

	unsigned long Suspend();//挂起线程，暂停执行
	unsigned long Resume(); //继续执行线程

	int WaitFor(unsigned long  ms ); //等待线程安全退出，如果超过ms时间未退出则强行杀死线程
	short Kill(unsigned long exitCode);//强制杀死线程，并设置线程退出码
	short IsRunning(); //线程是否在运行中

	int GetPriority(); //获取线程优先级
	int SetPriority(int NewPri);//设置线程优先级

	int GetExitCode(unsigned long * dwExitCode );//获得线程的退出码

	const char * Name(); //获得线程名称
	unsigned long LastActiveTime(); //上一次激活时间
	void NotifyQuit();	//通知线程退出，对于有大量线程的情况，可以加快线程退出速度
};


#endif //__E15_BaseObject_H

