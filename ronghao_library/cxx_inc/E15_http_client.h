#ifndef __E15_http_client_H
#define __E15_http_client_H

#include "E15_object.h"
#include "E15_string.h"
#include "E15_value.h"
#include "E15_socket.h"


class E15_cxx_object_API E15_HttpClient : public E15_Object
{
public:
    E15_HttpClient();
    virtual ~E15_HttpClient();

    void Init(E15_Socket * sockmgr);

    virtual void OnDisConnect();
    virtual void OnResponed(const char * url,int status,E15_ValueTable *& header,E15_String *& data,E15_Id * session_id) ;

    void SetReqId(E15_Id * id); //设置本次请求的session id
	void SetHost(const char * host); //设置头域中host的值

	int Connect(const char * addr, int port);
	int IsConnect();
	void Close();

	int Request(
		const char * method,/* GET/POST/DELETE....*/
		const char * url,
		const E15_ValueTable * headers,
		const E15_String * data);
};


#endif
