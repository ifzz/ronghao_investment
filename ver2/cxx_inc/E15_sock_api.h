#ifndef __E15_sock_api_H
#define __E15_sock_api_H

#include "E15_string.h"
#include "E15_sockaddr.h"

class E15_cxx_object_API E15_SockApi
{
public:
	static void Init();
	static int Error();
	static void SetError(int err);

	static int OpenUdp(const char * addr,int port ,int bindflag);
	static int OpenTcp(const char * addr,int port ,int bindflag);
	static int Connect(const char * addr,int port);

	static int OpenUdp(E15_SockAddr * addr,int bindflag);
	static int OpenTcp(E15_SockAddr * addr,int bindflag);
	static int Connect(E15_SockAddr * addr,int sock);

	static int Listen(int sock,int backlog);
	static int Accept(int sock,E15_SockAddr * addr);

	static void Close(int sock);

	static int Wait(int sock,int timeout_ms,int read_write_flag);

	static int Write(int sock,const char * data,int len);
	static int Send(int sock,E15_SockAddr * recvaddr,const char * data,int len);

	static int Read(int sock,char  * buffer,int len);
	static int RecvFrom(int sock,E15_String * buffer,E15_SockAddr * addr);

	static void Reuse(int sock,int flag);
	static void BroadCast(int sock,int flag);
	static void NoDelay(int sock,int flag);
	static void AllError(int sock,int flag);
	static void Block(int sock,int flag);

	static int GetRoutingInf(E15_SockAddr * remote,E15_SockAddr * local);

	static int GetLocalAddr(int sock,E15_SockAddr * addr);
	static int GetRemoteAddr(int sock,E15_SockAddr * addr);
};

#endif
