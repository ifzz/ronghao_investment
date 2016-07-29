#include "E15_socket.h"
#include "main.h"
#include "client_mgr.h"


E15_App::E15_App()
{
	m_server = 0;
	m_socket = 0;
}

void MainLog(int level,const char * fmt,va_list ap)
{
    E15_Log * log = (E15_Log*)E15_Debug::GetLogObj();
    log->PrintfV(level, fmt, ap);
}


int E15_App::OnInit(int argc,char * argv[])
{
	m_log.Init("client_mgr",100);
	E15_Debug::SetLogObj(&m_log);
	E15_Debug::SetLog( MainLog );

	m_socket = new E15_Socket;
	m_socket->Start();

	m_server = new Client_Mgr;
	g_server = m_server;
	m_server->Start(m_socket,"./ini/server.ini");

	return 1;
}


void E15_App::OnDestroy()
{
    m_server->Stop();
    m_socket->Stop();

    delete m_server;
    delete m_socket;

    E15_Debug::SetLog( 0 );
}


int main(int argc,char * argv[])
{
	E15_App app;

	app.Run(argc,argv);
	return 0;
}



