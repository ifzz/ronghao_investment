#ifndef __E15_zip_H
#define __E15_zip_H


#include "E15_object.h"
#include "E15_string.h"



class E15_cxx_object_API E15_Zip : public E15_Object
{
public:
	E15_Zip();
	virtual ~E15_Zip();

	void zip_start(E15_String * buffer);
	unsigned long zip(const char * data, long len); //finish压缩结束
	void zip_end();

    //解压缩
	void unzip_start(E15_String * buffer);
	unsigned long unzip(const char * data, long len); //finish压缩结束
	void unzip_end();

};


#endif
