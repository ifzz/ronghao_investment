/****************************************************
 * file E15_c_string.h dclare string interface      *
 * Author   E15(易尧武)                            *
 * Date     2013-10-14                              *
 * EMail	e15.com@126.com                     *
 * version	0.9.0                               *
 ****************************************************/
#ifndef __E15_queue_H
#define __E15_queue_H

#include <stdarg.h>
#include "E15_object.h"

class E15_Queue ;
class E15_QueueItem ;

/**************************************
  队列元素的接口
**************************************/

class E15_cxx_object_API E15_QueueItem : public E15_Object
{
public :
    E15_QueueItem();
    virtual ~E15_QueueItem();
	//获得元素所在队列的对象
	E15_Queue * Queue( );
	//元素脱离队列
	void Remove();
	//把元素移动到队头
	void ToHead( );
	//把元素移动到队尾
	void ToTail( );
	//获得排在自己后面的元素
	E15_QueueItem * Next();
	//获得排在自己前面的元素
	E15_QueueItem * Pre();
	//插入元素在自己前面（自己必须已经在队列中）
	E15_QueueItem * AddBefore(E15_QueueItem * newItem);
	//插入元素到自己后面（自己必须已经在队列中）
	E15_QueueItem * AddAfter(E15_QueueItem * newItem);
};

/**************************************
  队列的接口
 **************************************/

typedef int ( *E15_Queue_Compare)(E15_Object * obj1, E15_Object * obj2,void * param);

class E15_cxx_object_API E15_Queue : public E15_Object
{
public:
    E15_Queue(short thdSafe = 1,short event = 1);
    virtual ~E15_Queue();

	//初始化为线程安全和事件同步队列,请务必在队列为空的状态下操作，否则不保证安全性
	void SetThreadSafe(int thread_safe_flag); //队列是否线程安全
	void SetEventSync(int event_sync_flag); //队列进行事件同步

	//快速排序
	void QSort( int (*f)(E15_Object * , E15_Object * ,void * ) ,void * compare_param );
	//获得队列大小
	unsigned long Count();
	//删除所有队列元素
	void Clear();

	//等待队列可用,>0 队列可用
	int Wait(unsigned long ms);//等待队列有元素
	//设置队列可用信号
	void Set(); //触发等待返回

	//在指定位置插入数据，0表示在队列头，-1表示队尾，其他为队列中的位置，如果order_index >= 队列元素量，则插入队尾
	E15_QueueItem * InsertAt(E15_QueueItem *item,unsigned long z_order_index);
	//InsertAt的特殊参数函数
	E15_QueueItem * PutHead(E15_QueueItem *item);
	E15_QueueItem * PutTail(E15_QueueItem *item);

	//获取指定位置的队列元素，0表示在队列头，-1表示队尾，其他为队列中的位置，如果order_index >= 队列元素量，返回NULL
	E15_QueueItem * At(unsigned long z_order_index,unsigned long waitTime_ms );
	E15_QueueItem * Head(unsigned long waitTime_ms);
	E15_QueueItem * Tail(unsigned long waitTime_ms);

	//获得指定位置元素并且从队列移除该元素
	E15_QueueItem * RemoveAt(unsigned long z_order_index,unsigned long waitTime_ms);
	E15_QueueItem * RemoveHead(unsigned long waitTime_ms);
	E15_QueueItem * RemoveTail(unsigned long waitTime_ms);

	void RemoveAll();

	//遍历队列并执行相关函数，当f 返回值非0时，表示终止遍历操作
	void each( int (*f)(E15_Object *,void *), void * param);
	void Range(unsigned long z_order_index,int cnt,  int (*f)(E15_Object *,void *), void * param);
};



#endif
