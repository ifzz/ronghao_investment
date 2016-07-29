#ifndef __E15_timer_H
#define __E15_timer_H

#include "E15_object.h"

typedef struct E15_cxx_object_API E15_TimerInfo
{
	E15_Id *      timer_id;
    unsigned long repeat;
    unsigned long long elapse;
    short closed;
}E15_TimerInfo;

class E15_Timer;

class E15_cxx_object_API E15_TimerTriger : public E15_Object
{
public:
    E15_TimerTriger();
    virtual ~E15_TimerTriger();

    virtual void OnTimer(E15_Timer * timer,E15_TimerInfo * info) = 0;
    //对于重复性定时器，可以规律的改变时间，比如2x,3x,,,,,
    virtual unsigned long Delta(E15_Id * timer_id,unsigned long repeatCount,unsigned long raw_ms);

}; //该对象被删除前，一定要清除掉对应的定时器设置，避免多线程引起的崩溃。


class E15_cxx_object_API E15_Timer : public E15_Object
{
public:
    E15_Timer();
    virtual ~E15_Timer();

	void  Start(const char * name); //启动定时器服务
	void  Stop(); //停止定时器服务

	//设定一次的定时器,triger对象被删除前，一定要清除掉对应的定时器设置，避免多线程引起的崩溃。
	int SetTimer(unsigned long ms,E15_TimerTriger * triger,E15_Id * timerid = 0); //timerid用来存放设置成功后的id,如果timerid = 0,则不关心该值
	//设定重复多次的定时器，如果repeatCount = 0,则无限重复，
	int SetRepeatTimer(unsigned long repeatCount,unsigned long ms,E15_TimerTriger * triger,E15_Id * timerid = 0,int delta = 0); //时间变动是否生效

	void ClearTimer(const E15_Id * id);
	int ResetTimer(const E15_Id * id,unsigned long ms);

	unsigned long long StopWatch(const E15_Id * id,int del);

};


#endif
