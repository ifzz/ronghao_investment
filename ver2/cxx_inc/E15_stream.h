#ifndef __E15_stream_H
#define __E15_stream_H

#include <stdarg.h>

#include "E15_object.h"

class E15_cxx_object_API E15_Stream : public E15_Object
{
public:
    enum StreamType{
        Stream_File,
        Stream_Socket,
        Stream_Comm
    };

    E15_Stream();
    virtual ~E15_Stream();

	int Handle();
	int Error();
	void SetType( E15_Stream::StreamType type);

	//mod = 0:read
	//mod = 1:write
	//mod = 2:read and write
	void Attach(int fd);
	void Detach();
	long Seek(long offset,int from);

	int Read(char * data);
	int Read(short * data);
	int Read(int *data);
	int Read(long *data);
	int Read(float *data);
	int Read(double *data);

	int Read(unsigned char *data);
	int Read(unsigned short *data);
	int Read(unsigned int *data);
	int Read(unsigned long *data);
	int Read(long double *data);
	int Read(char * buff,int len);

	const char * ReadLine(int stripnl = 0,int MaxLength = -1);//default 0,-1

	int Write(char data,int cnt = 1 );//default 1
	int Write(short data);
	int Write(int data);
	int Write(long data);
	int Write(float data);
	int Write(double data);

	int Write(unsigned char,int cnt = 1);//default 1,
	int Write(unsigned short data);
	int Write(unsigned int data);
	int Write(unsigned long data);
	int Write(long double data);
	int Write(const char * data,int len = -1);//default -1,写入一个完整字符串

	int Flush();

	int Printf(const char * fmt,...);
	int PrintfV(const char * fmt,va_list ap);
	unsigned char last_char();


	void SetByteOrder(int order);
	void Skip(unsigned long len);
	int GetReadBuffCount();

	void SetBlockTime(int ms);
	int RawRead();	//数据可读后进行一次读操作
};

#endif
