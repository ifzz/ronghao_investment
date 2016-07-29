#pragma once

struct NODE_INFO {
	E15_Id id;
	std::set<std::string> ins_set;

	NODE_INFO(E15_Id node_id)
	:id(node_id) {}
};

class history_mgr;
class data_trans : public E15_Server {
public:
	data_trans(history_mgr *mgr_ptr);
	virtual ~data_trans() {}

public:
	int start();
	void stop();
	void send_data(E15_String *data, int cmd);

public:
	/* server callback */
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json);
	virtual int OnClose(E15_ServerInfo * info);
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}

private:
	std::list<std::shared_ptr<NODE_INFO>>::iterator find_node(E15_Id& id);
	int handle_subscribe(std::list<std::shared_ptr<NODE_INFO>>::iterator& it, int cmd, E15_String *&data);
	void send_ins_info(const char *ins, E15_Id& id);

private:
	history_mgr *m_mgr_ptr;
	E15_Id m_diagram_id;
	std::list<std::shared_ptr<NODE_INFO>> m_node_list;

	int m_market;
	E15_ValueTable m_instrument_list;
	E15_String m_instrument_buffer;
	E15_Zip m_zip;
};
