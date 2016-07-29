#pragma once

#include "crx_pch.h"
#include "E15_socket.h"
#include "E15_server.h"
#include "E15_ini.h"

struct node_sub {
	E15_Id id;
	std::set<std::string> subs;
};

struct server_role {
	std::string real_md;
	std::string his_md;
	std::string trade;
	std::string client_ui;
	std::string ope_mgr;
};

class trans : public E15_Server {
public:
	trans() :m_ins_list(nullptr), m_diagram_info(nullptr) {}
	virtual ~trans() {}

	void start();
	void stop();

public:
	/* server callback */
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json);
	virtual int OnClose(E15_ServerInfo * info);
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);

private:
	E15_String *m_ins_list, *m_diagram_info;
	std::list<std::shared_ptr<node_sub>> m_sub_list;		//客户端的订阅表
	std::map<std::string, E15_Id> m_role_id;
	server_role m_role;
};

class proxy : public crx::console {
public:
	proxy() {}
	virtual ~proxy() {}

public:
    virtual bool init(int argc,char * argv[]) {
    	m_trans.start();
    	return true;
    }

    virtual void destroy() {
    	m_trans.stop();
    	printf("proxy::OnDestroy!\n");
    }

private:
    trans m_trans;
};
