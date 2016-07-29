#ifndef __E15_file_H
#define __E15_file_H

#include "E15_object.h"


#ifdef WIN32
	#define E15_F_READ              0x00000000
	#define E15_F_WRITE             0x00000001
	#define E15_F_READWRITE         0x00000002
	#define E15_F_APPEND            0x00000008
	#define E15_F_CREATE            0x00000100
	#define E15_F_TRUNC             0x00000200
	#define E15_F_TEXT              0x00004000
	#define E15_F_BIN               0x00008000
#else
    #define E15_F_READ              00
    #define E15_F_WRITE             01
    #define E15_F_READWRITE         02
    #define E15_F_APPEND            02000
    #define E15_F_CREATE            0100
    #define E15_F_TRUNC             01000
    #define E15_F_TEXT              00
    #define E15_F_BIN               00

#endif

#define E15_FCOPY_SAFE			0
#define E15_FCOPY_OVERWRITE		1
#define E15_FCOPY_APPEND		2


class E15_cxx_object_API E15_File : public E15_Object
{
public:
    E15_File();
    virtual ~E15_File();

	long GetInfo(const char * filename);
	void Unlink();

	int IsExist();
	int IsDir();
	int IsModified();

	const char * FullName();
	const char * Path();
	const char * Name();

	const char * BaseName();
	const char * Suffix();

	unsigned long Size();

	long AccessTime();
	long ModifyTime();
	long CreateTime();

};

class E15_StringArray ;
class E15_String;

class E15_cxx_object_API E15_FileApi
{
public:
	static int Exist(const char * filename);
	static int  Open(const char * filename,int flag,int mod);
	static void Close(int fd);
	static long Seek(int fd,long offset,int from);
	static int Copy(const char * sourcefile,const char * target,int mode);
	static int Rename(const char * sourceFile,const char * target,int mode);

	static const char *	Temp(char *tempFilename);
	static unsigned long	Size(const char * pathname);
	static unsigned long	Size(int fd);

	static unsigned long	Lines(const char * pathname);
	static unsigned long	Lines(int fd);
	/*
		result data:
		[0] :: 相对路径
		[1] :: 文件名
		[2] :: 文件名不包含后缀部分
		[3] :: 文件后缀名
	*/
	static void	FileNameInfo(const char * pathname,E15_StringArray * result);
	static int	mkdir(const char * dir,int mode);
	static int	Type(const char * file); //目录 or file
	static int	Error();

	static const char * GetRealPathName(const char * file,E15_String * realpath);

	static int DeleteFiles(const char * path,const char * filter,int subdir =1 ); //default = 1
	static void DeleteTree(const char * path);
};



#endif
