#ifndef __E15_pool_H
#define __E15_pool_H

#include "E15_object.h"

class E15_cxx_object_API E15_Pool : public E15_Object
{
public:
	E15_Pool( E15_Object * (*obj_create)(void * p) ,unsigned int max_res = 0x10000);//默认65536个资源
	virtual ~E15_Pool();

	E15_Object * Alloc(void * params,E15_Id * rid); //随机分配
	E15_Object * Alloc(unsigned int index_z_order,void * params,E15_Id * rid); //从指定位置分配一个资源

	E15_Object * Peek(const E15_Id * rid); //精确查找

	//指定位置的方式，index 范围 [0,max_res)
	E15_Object * Peek(unsigned int index_z_order); //查看指定位置的对象
	unsigned int ToIndex(const E15_Id * rid); //获得id对应的资源位置编号

	unsigned int Counts(); //当前占用资源数量
	int Recycle(const E15_Id * rid);

	void RecycleAll();//把已经分配的资源全部回收

	//当pf返回非 0 ，表示终止当前循环,对已经分配资源进行一次遍历
	void each(int handler(E15_Object * obj,void * param),void * param);
};


#endif
