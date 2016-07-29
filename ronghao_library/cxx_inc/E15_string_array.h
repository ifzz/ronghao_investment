#ifndef __E15_StringArray_H
#define __E15_StringArray_H

#include <stdarg.h>

#include "E15_object.h"
#include "E15_string.h"

class E15_cxx_object_API E15_StringArray : public E15_Object
{
public:
    E15_StringArray();
    E15_StringArray(const E15_StringArray & a);

    virtual ~E15_StringArray();

	unsigned long Size();//返回数组大小

	E15_String * At(unsigned long nIndex) ; //返回指定位置的数据，nIndex = 0,返回第一个，-1，返回最后一个，超出范围，返回最后一个
	E15_String * Head() ;//返回第一个,Get的简化
	E15_String * Tail() ;//返回最后一个，Get的简化

	E15_String * Set(unsigned long nIndex, const char * newElement,unsigned long len = -1);//Get和memcpy的组合

	void Insert(unsigned long nIndex, const char * newElement, unsigned long nCount);//插入一个
	void Insert(unsigned long nStartIndex, const E15_StringArray* pNewArray);//插入数组

	E15_String * Add(const char * newElement,unsigned long len = -1);//增加一个到最后，如果newElement = NULL,增加一个空串
	E15_String * Add(const E15_String * data);//增加一个到最后，如果data = NULL,增加一个空串
	unsigned long Add( const E15_StringArray * src);//拷贝src到obj的末尾


	void Remove(unsigned long nIndex, unsigned long nCount);//从指定位置删除nCount个数据
	void RemoveTail();//删除最后一个数据
	void RemoveAll();//删除所有字符串

	unsigned long Split(const char * data,char sep,int conmode);//把字符串data按sep分割成数组，conmode！=0表示如果遇到连续的seq作为一个处理
	unsigned long Split(E15_String * pdata,char sep,int conmode);//把字符串data按sep分割成数组，conmode！=0表示如果遇到连续的seq作为一个处理

	void each(int on_string(E15_String * str,void * param),void * param);

};

#endif
