/*
    作者：易尧武
    邮件：e15.com@126.com
    版本: 2.0
    时间：2013.12.20
*/

/*
    简单xml格式(兼容windows ini格式)的配置文件读写
//	[section]
//
//		key = value
//		key = "this is test value"
//		key = 0x1234
//		key = 24.190
//
//
//	[section2]
//		<child-section>
//			<child-child-section2>
//				key =
//				key2 =
//			</child-child-section2>
//			...
//			<child-child-section2>
//			</child-child-section2>
//		</child-section>
//
//		<child-section>
//			<child-child-section2>
//			</child-child-section2>
//			....
//			<child-child-section2>
//			</child-child-section2>
//		</child-section>
//
//		<child-section2>
//			<child-child-section2>
//			</child-child-section2>
//			<child-child-section2>
//			</child-child-section2>
//		</child-section2>
*/

#ifndef __E15_config_H
#define __E15_config_H

#include <stdarg.h>

#include "E15_object.h"

class E15_cxx_object_API E15_Ini : public E15_Object
{
public:
    E15_Ini();
    virtual ~E15_Ini();

	int Read(const char *filename); //读配置文件
	int Dump(const char * filename); //写配置文件

	const char * FileName();//获得当前配置对应的配置文件名

	const char * SetSection(const char * fmt,...);//设置当前配置段位置
	const char * VSetSection(const char * fmt,va_list ap);//设置当前配置段位置
	const char * GetSection();//获得当前配置段位置

	int GetChildSectionCount(const char * name);//获得子section的数量
	const char * GetChildSectionName(int index);//获得子section的名字

	int GetKVCount();//获得配置项的数量
	const char * GetFirstKey();//获得第一个配置项的key
	const char * GetNextKey();//获得下一个配置项的key

	void RemoveSection();//删除配置段
	void RemoveChildSection(const char * secname,int index = 0); //default = 0，删除子配置段，如果有多个同名，则删除指定位置的配置段，当index为0，删除所有
	const char * ToChildSection(const char * secname,int index = 0); //default = 0,进入子配置段
	void ToParentSection();//返回上层配置段
	void ToTopSection();//返回顶层配置段

	void RemoveKey(const char * key);//删除配置项
	void AddSection(const char * name);//增加配置段

	int Read( const char * keyname,char & data);
	int Read(const char * keyname,short & data);
	int Read(const char * keyname,int & data);
	int Read(const char * keyname,long & data);
	int Read(const char * keyname,float & data);
	int Read(const char * keyname,double & data);
	int Read( const char * keyname,char * data,int len);

	int Read( const char * keyname,unsigned char & data);
	int Read(const char * keyname,unsigned short & data);
	int Read(const char * keyname,unsigned int & data);
	int Read(const char * keyname,unsigned long & data);
	int Read( const char * keyname,unsigned char * data,int len);

	const char * ReadString(const char * keyname,const char * def = 0); //default = 0

	int Write( const char * keyname,const char data);
	int Write(const char * keyname,const short data ,int hex = 0 );//default = 0
	int Write(const char * keyname,const int  data,int hex = 0);//default = 0
	int Write(const char * keyname,const long data ,int hex = 0 );//default = 0
	int Write(const char * keyname,const float  data);
	int Write(const char * keyname,const double data);
	int Write(const char * keyname,const char * data);

	int Write(const char * keyname,const unsigned char data);
	int Write(const char * keyname,const unsigned short data,int hex =0 );//default = 0
	int Write(const char * keyname,const unsigned int data,int hex =0 );//default = 0
	int Write(const char * keyname,const unsigned long data,int hex =0 );//default = 0

};

#endif
