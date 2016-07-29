#include "E15_console.h"

#include "E15_debug.h"
#include "E15_log.h"


class E15_Socket;
class Client_Mgr;

class E15_App : public E15_ConsoleApp
{
public:
    E15_App();

    virtual int OnInit(int argc,char * argv[]);
    virtual void OnDestroy();

public:
	E15_Socket * m_socket;
	Client_Mgr  * m_server;
  E15_Log    m_log;

};


