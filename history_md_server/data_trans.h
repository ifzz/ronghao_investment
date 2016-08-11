#pragma once

#include "rhafx.h"

class history_mgr;
class data_trans : public E15_Server {
public:
	data_trans(history_mgr *mgr_ptr);
	virtual ~data_trans() {}

public:
	int start();
	void stop();
	void send_data(const std::string& ins_id, E15_String *data, int cmd);

public:
	/* server callback */
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json);
	virtual int OnClose(E15_ServerInfo * info);
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}

private:
	int handle_subscribe(int cmd, E15_String *&data, const std::string& name);
	void send_ins_info(const char *ins, E15_Id& id);

private:
	history_mgr *m_mgr_ptr;
	std::string m_stg_role, m_dia_role;
	E15_Id m_stg_id, m_dia_id;
	std::map<std::string, std::set<std::string>> m_node_sub;

	int m_market;
	E15_ValueTable m_instrument_list;
	E15_String m_instrument_buffer;
	E15_Zip m_zip;
};
