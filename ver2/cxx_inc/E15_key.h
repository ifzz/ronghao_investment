#ifndef __E15_key_H
#define __E15_key_H

#include "E15_string.h"
///////////////////////////
// key
///////////////////////////
typedef enum E15_KeyType {
	E15_KeyType_Unvalid = 0,
	E15_KeyType_I,
	E15_KeyType_L,
	E15_KeyType_II,
	E15_KeyType_String,
	E15_KeyType_IS,
	E15_KeyType_LS,
	E15_KeyType_IIS
}E15_KeyType;


class E15_cxx_object_API E15_Key : public E15_Object
{
public:
    E15_Key();
    virtual ~E15_Key();

	//序列化处理
	unsigned long Dump(E15_String * buf);
	unsigned long Import(const char * bytearray,int maxlen);

	unsigned long Hash(int nocase = 0);//计算hash值
	void SetNoCase(int nocase ); //设置字符串部分是否区分大小写

	void SetS(const char * skey,int len,int nocase); //设置为字符串key
	void SetI(unsigned int ukey,const char * name);  //设置为整形Key
	void SetL(unsigned long ukey,const char * name); //设置为长整形key
	void SetII(unsigned int hd,unsigned int ld,const char * name);//设置为双int key

	void SetIS(unsigned int ukey,const char * skey,int nocase = 0); //设置字符串+int
	void SetLS(unsigned long ukey,const char * skey,int nocase = 0); //字符串+long
	void SetIIS(unsigned int hd,unsigned int ld,const char * skey,int nocase = 0); //字符串+int+int

	E15_KeyType GetType();

	unsigned int GetIH();
	unsigned int GetIL();
	unsigned long GetL();
	const char * GetS();

	int EqualI(unsigned int ukey);
	int EqualL(unsigned long ukey);
	int EqualII(unsigned int h,unsigned int l);
	int EqualS(const char * skey,int nocase = 0);
	int EqualLS(unsigned long ukey,const char * skey,int nocase = 0);

	//快速加密算法
	unsigned long Encrypt(const char * data,unsigned long len,E15_String * outbuf);//加密
	unsigned long Decrypt(const char * data,unsigned long len,E15_String * outbuf);//解密

    static unsigned long HashString(const char * s,int nocase = 0);
    static unsigned long HashLong(unsigned long l);
    static unsigned long HashLS(unsigned long ukey, const char * s,int nocase = 0);

};



#endif
