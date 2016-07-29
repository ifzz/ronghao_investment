#ifndef __E15_log_H
#define __E15_log_H


#include <stdarg.h>

#include "E15_object.h"

class E15_cxx_object_API E15_Log : public E15_Object
{
public:
	E15_Log();
	E15_Log(int threadSafe );
	virtual ~E15_Log();

	void Init(const char * pre,int level ); //default 1000
	void SizeLimit(int s_limit);
	void LineLimit(int l_limit);


	void Printf(int level,const char * fmt,...);
	void PrintfV(int level,const char * fmt,va_list ap);

	void Printf(long itime,int level,const char * fmt,...);
	void PrintfV(long itime,int level,const char * fmt,va_list ap);

	void HexPrint(int level,const char * data,int datalen,const char * infofmt,...);
	void HexPrintV(int level,const char * data,int datalen,const char * infofmt,va_list ap);

	void Write(const char * data);
	void Level(int level);

	void MainDir(const char * dir);

	const char * DirInfo();

	void	FFlush();
	void	CheckFile();
};



#endif
