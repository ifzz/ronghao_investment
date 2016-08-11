/****************************************************
 * file E15_string.h declare string interface      *
 * Author   E15(易尧武)                            *
 * Date     2015-10-14
 *
 * EMail	e15.com@126.com                     *
 * version	0.9.0                               *
 ****************************************************/
#ifndef __E15_string_H
#define __E15_string_H

#include <stdarg.h>
#include "E15_object.h"


class E15_cxx_object_API E15_String : public E15_Object
{
public :
    E15_String();
    E15_String( const E15_String & );
    E15_String(const char * );
    virtual ~E15_String();

	int Equal(const E15_String & ,int nocase  = 0 ); //nocase =1,不区分大小些
	int Equal(const char * b,int nocase = 0);//判断对象的值是否与字符串相等，如果nocase为真，则不区分大小写

	E15_String & truncate(unsigned long len); //字符串截到len长度
	E15_String & truncate_tail(unsigned long len);//截掉尾部的len长度字符，保留长度为 xx - len
	const char * tail(unsigned long len);//获取尾部len长度的字符串,相当于 c_str() + (xx - len);

	E15_String & Strcpy(const char * data,unsigned long len = -1);//拷贝最大长度len个字符,len < 0 ,拷贝整个字符串
	E15_String & Strcat( const char * data,unsigned long len = -1); //追加拷贝最大len个字符，len < 0 ,拷贝整个字符串


	E15_String &  Sprintf(const char * fmt,...);	//格式化数据
	E15_String &  SprintfV(const char * fmt,va_list ap);

	E15_String &  Append(const char * fmt,...);//追加格式化数据
	E15_String &  AppendV(const char * fmt,va_list ap);

	E15_String & Memcpy( const char * data,unsigned long len); //拷贝功能
	E15_String & Memcat( const char * data,unsigned long len);

	E15_String &  Insert (unsigned long offset,const char * ,...); //在指定位置格式化插入字符串
	E15_String & InsertV(unsigned long offset,const char * ,va_list);

	E15_String & Insert(unsigned long offset,char ch,unsigned long count = 1);//在指定位置插入字符
	E15_String & InsertStr(unsigned long offset,const char * str,unsigned long len);//在指定位置插入字符串

	E15_String & Del(unsigned long offset,unsigned long count); //在指定位置删除数据

	E15_String & Replace(char ch,char newch); //替换字符
	E15_String & Addch(char ch,unsigned long cnt = 1);//追加cnt个字符

	E15_String & Resize(unsigned long count, int  ch = -1 ); //调整字符串长度，如果比原始数据大，则ch为添加的数据值,如果ch >=0 && <=255,则新增部分采用该值填充
	char * Extend(unsigned int len);

	const char * c_str(); //获得字符串

	long Length(); //获得字符串长度

	E15_String & TrimLeft(); //删除前导的空白字符
	E15_String & TrimRight();//删除尾部的空白字符
	E15_String & Trim();	//删除前后的空白字符
	unsigned long FindLastChar(char ch);//从尾部逆向开始查找ch,

	//offset为输入输出参数，指定搜索的开始偏移，搜到子串后为字串的开始位子
	const char * Find(unsigned long *offset,const char * substr,unsigned long sublen); //default sublen = 0，从指定位置开始查找字串
	const char * FindLast(unsigned long * offset,const char * substr,unsigned long sublen ); //default sublen = 0，从尾部开始查找字串
	const char * MatchBrack(unsigned long start_offset,const char * begin_flag,const char * end_flag,unsigned long * begin_offset, unsigned long  *end_offset);//匹配括号或者标记

	E15_String & Upper(); //转换为大写
	E15_String & Lower(); //转换为小写

	short  GetAt(unsigned long nIndex) ; //获得指定位置的字符,< 0 则获取失败
	E15_String &    SetAt(unsigned long nIndex, char ch); //设置指定位置的字符

	E15_String &  Reverse (); //折返字符串

	const char * GetQuote();//获得字符串的引号标记
	E15_String &  SetQuote(char quote[2] ); //设置字符串引号标记, quote[0]开始标记，quote[1]结束标记
	E15_String &  Host2Net(const char * data,unsigned long len);//网络字节转换并追加到末尾

	E15_String & Align(char align_fmt, char filled_char, unsigned long str_length);//对齐

	//时间相关
	E15_String & FormatDate(long sec,const char * fmt = "yyyy-mm-dd hh:mi:ss" );//格式化时间,if fmt = 0,default == "yyyy-mm-dd hh:mi:ss"，
	E15_String & AppendDate(long sec,const char * fmt = "yyyy-mm-dd hh:mi:ss");
	long         ToTime(const char * fmt = "yyyy-mm-dd hh:mi:ss" ) ;//按照指定格式转化时间

	//字符串与数值的转换，主要是处理配置或者转义字符等情况
	long  long       ToInt( int format = 10) ; //字符串转化为数值，10 = DEC,16=HEX,2=BIN
	double       ToFloat() ;//转化为浮点
	char         ToChar() ;	//转换为字符,支持 10进制，16进制0x??的格式
	long  	     GetTimePeriod(const char * timefmt); //获得时间格式的周期yyyy的周期为年,yyyy-mm周期为月，yyyy-mm-dd 为天等等


	const char * Base64Encode(const char * rawdata,unsigned long len);//编码为base64,把rawdata编码为base64格式存放在obj中，
	const char * Base64Decode(const char * encdata,unsigned long len);//base64解码

	//字符集转换
	int  utf8_2_unicode_onece(const unsigned char *sraw, unsigned long n);//成功转换一个utf8即停止

	unsigned long utf8_2_unicode (const char *s, unsigned long n);
	unsigned long unicode_2_utf8 ( const char *s, unsigned long n);

	unsigned long utf8_2_gb18030 (  const char *s, unsigned long n);
	unsigned long gb18030_2_utf8( const char *s, unsigned long n);

	unsigned long unicode_2_gb18030( const char *s, unsigned long n);
	unsigned long gb18030_2_unicode (const char *s, unsigned long n);

	//快速加密算法
	unsigned long enc(const char * data,unsigned long len,const char *key);//加密(快速加密算法)
	unsigned long dec(const char * data,unsigned long len,const char *key);//解密(快速加密算法)

	E15_String &  Write(char );
	E15_String &  Write(unsigned char );
	E15_String &  Write(short  );
	E15_String &  Write(unsigned short );
	E15_String &  Write( int );
	E15_String &  Write(unsigned int );
	E15_String &  Write(long );
	E15_String &  Write(unsigned long  );
	E15_String &  Write(float );
	E15_String &  Write(double);
	E15_String &  Write(long long );

	E15_String &  Write(char * data,int cnt );
	E15_String &  Write(unsigned char*data,int cnt );
	E15_String &  Write(short * data,int cnt  );
	E15_String &  Write(unsigned short * data,int cnt );
	E15_String &  Write( int * data,int cnt);
	E15_String &  Write(unsigned int *data,int cnt );
	E15_String &  Write(long * data,int cnt);
	E15_String &  Write(unsigned long * data,int cnt );
	E15_String &  Write(float * data,int cnt);
	E15_String &  Write(double* data,int cnt);
	E15_String &  Write(long long * data,int cnt);

};

E15_cxx_object_API unsigned long HostNetExchange(const char * rawdata,char * out_buf,unsigned long len);//网络字节转换

#define E15_f_ReadVal(net_buf,value) \
do\
{\
	long E15_f_val_temp_len;\
	E15_f_val_temp_len = sizeof( value ) ;\
	unsigned char * E15_f_read_var_temp_pch = (unsigned char * )&(value );\
	E15_f_read_var_temp_len = host_net_exchange(net_buf,(char *)E15_f_read_var_temp_pch,E15_f_read_var_temp_len);\
	net_buf += E15_f_read_var_temp_len;\
}\
while(0);

#define E15_f_ReadLong(net_buf,value,net_long_size) \
do\
{\
	if( sizeof(long) == (net_long_size) )\
	{\
		unsigned char * pch = (unsigned char * )( &(value) );\
		host_net_exchange((net_buf),(char *)pch,net_long_size);\
		(net_buf) += net_long_size;\
		break;\
	}\
	if( net_long_size == 4 )\
	{\
		unsigned int E15_f_temp_len = 0;\
		unsigned char * E15_f_temp_pch = (unsigned char * )( &E15_f_temp_len );\
		host_net_exchange(net_buf,(char *)E15_f_temp_pch,4);\
		net_buf += 4;\
		value = E15_f_temp_len;\
		break;\
	}\
	if( net_long_size == 8 )\
	{\
		unsigned char * E15_f_temp_pch = (unsigned char * )( &(value) );\
		net_buf += 4;\
		host_net_exchange(net_buf,(char *)E15_f_temp_pch,4);\
		net_buf += 4;\
		break;\
	}\
}\
while(0);

#define E15_f_WriteVal(net_buf,value) \
do\
{\
	long E15_f_temp_len;\
	E15_f_temp_len = sizeof(value);\
	const char * E15_f_temp_pch = (const char * )&(value);\
	E15_f_temp_len = host_net_exchange(E15_f_temp_pch,net_buf,E15_f_temp_len);\
	net_buf += E15_f_temp_len;\
}\
while(0);

#define E15_f_WriteValType(net_buf,type,c_val) \
do\
{\
	long E15_f_temp_len;\
	E15_f_temp_len = sizeof(type);\
	type value = c_val;\
	const char * E15_f_temp_pch = (const char * )&(value);\
	E15_f_temp_len = host_net_exchange(E15_f_temp_pch,net_buf,E15_f_temp_len);\
	net_buf += E15_f_temp_len;\
}\
while(0);


#endif
