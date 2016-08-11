#ifndef __E15_debug_H
#define __E15_debug_H

#include "E15_object.h"

#include <stdarg.h>


class E15_cxx_object_API E15_Debug
{
public:
	static void Init();
	static void UnInit();

	static void Charset(int charset); // 0 utf8; 1 gb18030
	static void Printf(int level,const char * fmt,...); //界面输出函数，默认不输出
	static void PrintfV(int level,const char * fmt,va_list); //界面输出函数，默认不输出

	static void PrintfNT(int level,const char * fmt,...); //界面输出函数，默认不输出
	static void PrintfVNT(int level,const char * fmt,va_list); //界面输出函数，默认不输出

	static void Log(int level,const char * fmt,...); //日志输出函数，默认输出到系统日志
	static void LogV(int level,const char * fmt,va_list); //日志输出函数，默认输出到系统日志

	static void SetPrint(void (* pfunc)(int level,const char * fmt,va_list));//更改打印输出方式，如果 pfunc == 0 ,恢复默认
	static void SetLog(void (* pfunc)(int level,const char * fmt,va_list) );//更改日志输出方式，pfunc == 0 ,恢复默认

	static void SetPrintObj(void * obj);
	static void SetLogObj(void * obj);

	static void *GetPrintObj();
	static void *GetLogObj();
};


#endif
