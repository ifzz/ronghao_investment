#ifndef __E15_reorder_H
#define __E15_reorder_H

#include "E15_object.h"


class E15_cxx_object_API E15_Reorder : public E15_Object
{
public:
    E15_Reorder();
    virtual ~E15_Reorder();

	void Init(int bufsize, int max_wait,int delete_flag = 1 );//初始化缓冲大小，对象删除函数,最大延迟数
	void Reset();

	void SetFirstSeq(unsigned int seq); //可以设置期望收到或者发送的第一个seq
	void SetMaxwait(int maxwait);//设置最大延迟数(对于接收缓冲有用)

	//用作voip等流媒体数据接收缓冲，对于允许丢包以及本地补偿的情况,如果data == 0,追加到末尾,返回值大于0才成功（data)被使用
	int CacheItem(unsigned int seq, E15_Object * data,int discard );
	//从缓冲区接收数据，cache_size表示队列中需要等待的数据数量,当数据个数小于cache_size时，不返回数据
	E15_Object * GetItemWait(unsigned int * seq,int cache_size,int * wait_flag );


	//用作重要数据传输的接收，保证数据完整性，必须网络重传来确保数据完整性
	int SafeCache(unsigned int seq, E15_Object * data);//
	E15_Object * SafeGet(unsigned int * seq); //获得有效数据
	unsigned int GetNok(); //当前正期待的数据的seq

	//完整性数据传输的发送控制
	int SafeWait(unsigned int * seq,E15_Object * data);// 自动排到最大序列号
	unsigned int GetNck(); //获取当前最新未ack数据的seq)

	//on_data 返回1，表示data已经被上层代码处理掉，无须底层调用delete动作
	int Ack(unsigned int seq,int on_data(unsigned int seq,E15_Object * data,void * params),void * params );//对序列号 在seq之前（含seq) 的数据进行确认
	unsigned int GetSendSeq(); //获取下一个发送数据的seq
	int IsEmpty(); //是否有未确认的发送数据
	int IsFull(); //是否有未确认的发送数据
	E15_Object * PeekData(unsigned seq); //是否有未确认的发送数据
	void each( int on_data(unsigned int seq,E15_Object * data,void * params),void * params ); //对缓冲区中的数据进行处理。

};

#endif

