#ifndef __E15_sockaddr_H
#define __E15_sockaddr_H

#include "E15_object.h"


struct sockaddr;

class E15_cxx_object_API E15_SockAddr : public E15_Object
{
public:
    E15_SockAddr();
    ~E15_SockAddr();

	//这个判断存在问题，对于ipv4和ipv6格式如何判断还需要研究
	int Equal(const char * ip);
	void Exchange( E15_SockAddr * b);//交换两个对象内容//为了减少一些拷贝而设置该函数

	//地址可以是ipv4,ipv6,unix sock,也可以是域名
	int Init(const char * addr,int port ); //port default 0
	int Init(struct sockaddr * addr,int addrlen);
	void ReInit();

	const char * ip();
	int port();

	struct addrinfo * Info();
	unsigned long sin_addr(int host_flag = 0);
	int sin_port(int host_flag = 0);
};

#endif
