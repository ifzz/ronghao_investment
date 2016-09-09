#ifndef __HttpGateway_H
#define __HttpGateway_H

#include "E15_string.h"
#include "E15_queue.h"
#include "E15_socket.h"
#include "E15_thread.h"
#include "E15_value.h"

class HttpGateway
{
public :
    HttpGateway();
    ~HttpGateway();

public:
	void Start(E15_Socket * mgr,const char * server,int port);
	void Stop();

    void Get(const char * key,const char * url,const char * json,void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data) = 0 );
    void Post(const char * key,const char * url,const char * json,void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data) = 0 );

private:
    E15_Queue * m_free;
    E15_Queue * m_busy;
    E15_Queue * m_task;

    E15_Thread * m_thd;

private:
    unsigned long doHandler();
    static unsigned long TaskHandler(void*);
private:
    friend class HttpClient;
};

#endif
