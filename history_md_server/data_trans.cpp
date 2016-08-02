#include "stdafx.h"

E15_Socket g_socket;

data_trans::data_trans(history_mgr *mgr_ptr)
:m_mgr_ptr(mgr_ptr)
,m_market(-1) {
}

int data_trans::start() {
	g_socket.Start();
	Start(&g_socket, "ini/server.ini");
	return 1;
}

void data_trans::stop() {
	Stop();
	g_socket.Stop();
	m_node_list.clear();
}

void data_trans::send_data(E15_String *data, int cmd) {
	E15_ServerCmd sc;
	sc.cmd = cmd;
	sc.status = 1;
	Notify(&m_diagram_id, 0, &sc, data->c_str(), data->Length());
	delete data;
}

int data_trans::OnOpen(E15_ServerInfo * info,E15_String *& json) {
	print_thread_safe("[%x:%x] (N=%d,name=%s:role=%s) 上线\n", info->id.h,
			info->id.l, info->N, info->name, info->role);

	if (!strcmp(info->role, STRATEGY_NODE) || !strcmp(info->role, STRATEGY_PROXY)) {
		m_node_list.push_back(std::make_shared<NODE_INFO>(info->id));
		print_thread_safe("[history_md]连接到策略服务器(%x:%x, name=%s, role=%s)\n", info->id.h,
				info->id.l, info->name, info->role);
	}

	if (!strcmp(info->role, MARKET_DATA_NODE)) {
		m_diagram_id = info->id;
		print_thread_safe("[history_md]连接到指标生成服务器(%x:%x, name=%s, role=%s)\n", info->id.h,
				info->id.l, info->name, info->role);
	}

	send_ins_info("600030", info->id);		//任何节点与该服务器连接时都发送合约列表信息

	//初始化完成后自动发送id为600030的合约用来测试
//	m_mgr_ptr->history_subscribe("600030", 20160628, 20160628, 500);
	return 1;
}

int data_trans::OnClose(E15_ServerInfo * info) {
	print_thread_safe("[%x:%x] (N=%d, name=%s:role=%s) 下线\n", info->id.h, info->id.l,
			info->N, info->name, info->role);

	if (!strcmp(info->role, STRATEGY_NODE)) {
		auto it = find_node(info->id);
		if (m_node_list.end() != it)
			m_node_list.erase(it);
	}

//	m_mgr_ptr->history_unsubscribe("600030");
	return 1000;		//自动重联
}

std::list<std::shared_ptr<NODE_INFO>>::iterator data_trans::find_node(E15_Id& id) {
	auto it = m_node_list.begin();
	for (; it != m_node_list.end(); ++it)
		if ((*it)->id == id)
			break;
	return it;
}

void data_trans::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
	auto it = find_node(info->id);
	switch(cmd->cmd) {		//只要接到请求就相应
	case Stock_Msg_SubscribeById:
	case Stock_Msg_UnSubscribeById:
		handle_subscribe(it, cmd->cmd, data);
		break;

	default:
		break;
	}
}

int data_trans::handle_subscribe(std::list<std::shared_ptr<NODE_INFO>>::iterator& it, int cmd, E15_String *&data) {
	E15_ValueTable vt;
	vt.Import(data->c_str(), data->Length());
	E15_Value *v = vt.ValueS("id_list");
	if (!v) {
		print_thread_safe("[data_trans]handle subscribe by id error, not find id_list, req data len=%d\n", data->Length());
		return -1;
	}
	E15_StringArray *sa = v->GetStringArray();

	unsigned int  start, end, interval;
	if (Stock_Msg_SubscribeById == cmd) {
		v = vt.ValueS("start_date");
		start = v->GetUInt();

		v = vt.ValueS("end_date");
		end = v->GetUInt();

		v = vt.ValueS("interval");
		interval = v->GetUInt();
	}

	for (unsigned long i = 0; i < sa->Size(); ++i) {
		E15_String *s = sa->At(i);
		if (Stock_Msg_UnSubscribeById == cmd) {		//取消订阅
			if ((*it)->ins_set.end() != (*it)->ins_set.find(s->c_str())) {
				(*it)->ins_set.erase(s->c_str());
				m_mgr_ptr->history_unsubscribe(s->c_str());
			}
		} else {		//Stock_Msg_SubscribeById == cmd
			if ((*it)->ins_set.end() == (*it)->ins_set.find(s->c_str())) {
				(*it)->ins_set.insert(s->c_str());
				m_mgr_ptr->history_subscribe(s->c_str(), start, end, interval);
				send_ins_info(s->c_str(), m_diagram_id);
			}
		}
	}
	return sa->Size();
}

void data_trans::send_ins_info(const char *ins, E15_Id& id) {
	E15_ValueTable *vt = m_instrument_list.InsertTableS(ins);
	m_market = MarketCodeById(ins);
	vt->SetSI("market", m_market);
	vt->SetSS("name", ins);
	vt->SetSI("tick", 100);
	vt->SetSI("Multiple", 1);
	vt->SetSS("exchange", "sh");
	vt->SetSS("product", ":CNA");

	E15_String s;
	m_instrument_list.Dump(&s);

	m_zip.zip_start(&m_instrument_buffer);
	m_zip.zip(s.c_str(), s.Length());
	m_zip.zip_end();

	if( m_instrument_buffer.Length() > 0 )
	{
		E15_ServerCmd  cmd;
		cmd.cmd = Stock_Msg_InstrumentList; //合约代码信息
		cmd.status = m_market;
		Notify(&id,0,&cmd,m_instrument_buffer.c_str(),m_instrument_buffer.Length(),1 );
	}
	m_instrument_list.Reset();
	m_instrument_buffer.Reset();
}
