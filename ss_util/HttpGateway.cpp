#include "rhafx.h"
#include "E15_http_client.h"
#include "E15_debug.h"

class HttpClient;


HttpQi::HttpQi(HttpGateway * gateway)
{
    gw = gateway;
    http = new HttpClient(this);
}
HttpQi::~HttpQi()
{
    delete http;
}


HttpClient::HttpClient(HttpQi * qItem)
{
    qi = qItem;
}

HttpClient::~HttpClient()
{
    Close();
}

void HttpClient::OnResponed(const char * url,int status,E15_ValueTable *& header,E15_String *& data,E15_Id * session_id)
{
    if( qi->OnResponed )
        qi->OnResponed(qi->key.c_str(),status,header,data);

    qi->gw->m_free->PutTail(qi);
    //可用于下一个http请求
}

void HttpClient::OnDisConnect()
{
    //E15_Debug::Printf(0,"OnDisConnect()\n");
    qi->gw->m_free->PutTail(qi);
}

HttpGateway::HttpGateway()
{
    m_free = new E15_Queue;
    m_busy = new E15_Queue(1,0);
    m_task = new E15_Queue;

	m_thd = 0;
}

void HttpGateway::Start(E15_Socket * mgr,const char * server,int port)
{
    for( int i=0; i < 128; i++)
    {
        HttpQi * qi = new HttpQi(this);

        qi->http->Init(mgr);
        qi->http->Connect(server,port);

        m_free->PutTail(qi);
    }

    m_thd = new E15_Thread("http_gw");
    m_thd->Init(TaskHandler,this);
    m_thd->Resume();
}

void HttpGateway::Stop()
{
	m_thd->WaitFor(2000);
	m_busy->RemoveAll();
	m_task->RemoveAll();
	m_free->RemoveAll();
}

HttpGateway::~HttpGateway()
{
	m_thd->WaitFor(2000);
    delete m_thd;

    delete m_busy;
    delete m_task;
    delete m_free;
}

void HttpGateway::Get(const char * key,const char * url,const char * json,void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data) )
{
    HttpTask * task = new HttpTask;
    task->url.Strcpy(url);
    task->json.Strcpy(json);
    task->OnResponed = OnResponed;
    task->key = key;
    task->method = 0;

    m_task->PutTail(task);
}

void HttpGateway::Post(const char * key,const char * url,const char * json,void (*OnResponed)(const char * key,int status,E15_ValueTable *& header,E15_String *& data) )
{
    HttpTask * task = new HttpTask;
    task->url.Strcpy(url);
    task->json.Strcpy(json);
    task->OnResponed = OnResponed;
    task->key = key;
    task->method = 1;

    m_task->PutTail(task);
}

unsigned long HttpGateway::TaskHandler(void*p)
{
    HttpGateway * gw = (HttpGateway*)p;
    return gw->doHandler();
}

unsigned long HttpGateway::doHandler()
{
    HttpQi * qi = 0;
    HttpTask * task = 0;
    while( m_thd->IsRunning() )
    {
        qi = (HttpQi*)m_free->RemoveHead(50);
        if( !qi )
            continue;
        m_busy->PutTail(qi);
        if( !qi->http->IsConnect() )
        {
			//
        }

        while( m_thd->IsRunning() )
        {
            task = (HttpTask *)m_task->RemoveHead(50);
            if( !task )
                continue;
			qi->key = task->key;
            qi->OnResponed = task->OnResponed;
            if( task->method )
				qi->http->Request( "POST",task->url.c_str(),0,&task->json);
			else
				qi->http->Request( "GET",task->url.c_str(),0,&task->json);
            delete task;
            break;
        }
    }
    return 0;
}

HttpTask::HttpTask()
{
    OnResponed = 0;
}
HttpTask::~HttpTask()
{
}
