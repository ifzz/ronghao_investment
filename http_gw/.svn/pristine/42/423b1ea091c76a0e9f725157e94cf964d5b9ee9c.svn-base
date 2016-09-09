#include "HttpGateway.h"

#include "E15_http_client.h"

class HttpClient;

class HttpQi : public E15_QueueItem
{
public:
    HttpQi(HttpGateway * gateway);
    virtual ~HttpQi();
public:
    HttpClient * http;
    HttpGateway * gw;
    E15_String   key;
    void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data);
};


class HttpClient : public E15_HttpClient
{
public:
    HttpClient(HttpQi * qItem);

    virtual ~HttpClient();

    void OnResponed(const char * url,int status,E15_ValueTable *& header,E15_String *& data,E15_Id * session_id);
    void OnDisConnect();
private:
    HttpQi * qi;
};

class HttpTask : public E15_QueueItem
{
public:
    HttpTask();
    virtual ~HttpTask();
public:
    E15_String url;
    E15_String json;
    E15_String   key;
    int method;
    void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data);
};



