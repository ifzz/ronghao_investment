#ifndef __E15_system_H
#define __E15_system_H

#include "E15_string.h"

struct tm ;
class E15_cxx_object_API E15_SystemApi
{
public:
    enum ByteOrder
    {
        SameHost	= 0,	//特殊的类型，表示同主机
        BigEndians ,
        LittleEndians ,
        MiddleEndians ,
    };

	static const char *  ExePathName(E15_String *oname);
	static const char *  ExeName(E15_String *oname);
	static const char *  ExePath(E15_String *oname);

	static const char *  CurrentDir(E15_String *oname);

	static long     gettimeofday(long * tv_sec,long * tv_usec);

	static double  difftimeval(long tv_sec1,long tv_usec1,long tv_sec2,long tv_usec2);

	static struct tm * localtime(long *timep,struct tm * ptm );

	static unsigned long   GetTickCount();
	static unsigned long   Sleep(long ms);

	static int GetDiskInfo(const char *path,long long *blocksize,long long *block_count,long long * block_free);

	static void * LoadDll(const char * dllname);
	static void   UnloadDll(void * dll);
	static void * Function(void * dll,const char * func_name);
	static ByteOrder	  GetByteOrder();	//主机字节顺序类型
	static unsigned long CurrentThreadId();
	static unsigned long CurrentProcessId();

	static unsigned int NextDate(unsigned int date);
	static unsigned int PreDate(unsigned int date);
	static int GetDayWeek(unsigned int date);
	static int DateDiff(unsigned int date1,unsigned int date2);
};


#endif
