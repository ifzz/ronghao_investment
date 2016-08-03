#include "stdafx.h"

int file_receiver::connect_file_server() {
	Init(&g_socket);
	int ret_code = Connect(FILE_SERVER_IP, FILE_SERVER_PORT);
	print_thread_safe(g_log, "成功启动本地http客户端用于接收远程策略so以及ini\n");
	return ret_code;
}

int file_receiver::request_file_by_url(const std::string& url) {
	if (!IsConnect())
		return 0;

	auto args = crx::split(url, " ");
	Request("GET", args[0].c_str(), 0, 0);
	Request("GET", args[1].c_str(), 0, 0);
	m_ini_so[args[1]] = args[0];
	print_thread_safe(g_log, "请求文件 url=%s\n", url.c_str());
	return 0;
}

void file_receiver::OnResponed(const char * url, int status, E15_ValueTable *& header,
		E15_String *& data, E15_Id * session_id) {
	if (200 != status) {
		print_thread_safe(g_log, "文件请求失败 url=%s\n", url);
		return;
	}

//	std::string file = url;
//	std::string local_file = STRATEGY_DIR+file.substr(file.rfind("/")+1);
//
//	FILE *p = fopen(local_file.c_str(), "w");
//	fputs(data->c_str(), p);
//	fclose(p);
//
//	if (std::string::npos != file.rfind(".ini")) {		//已同时取到so和ini文件
//		std::string so_file = m_ini_so[file];
//		so_file = STRATEGY_DIR+so_file.substr(so_file.rfind("/")+1);
//		std::vector<std::string> args = {so_file, local_file};
//		m_data_mgr->load_strategy(args);
//	}
}

data_manager::data_manager(strategy_manager *data_mgr)
:m_data_mgr(data_mgr) {
	m_file_receiver = new file_receiver(this);
}

data_manager::~data_manager() {
	delete m_file_receiver;
}

int data_manager::connect_data_server() {
#ifdef RUN_AS_CLIENT
	Start(&g_socket);

	E15_ClientInfo client;
	memset(&client, 0, sizeof(client));
	client.name = g_conf.user.c_str();
	client.pwd = g_conf.passwd.c_str();
	client.role = "strategy_client";
	Login(&client, "", g_conf.ip.c_str(), g_conf.port);
#else
	Start(&g_socket, "ini/socket.ini");
#endif
	print_thread_safe(g_log, "成功连接行情服务器\n");
	return m_file_receiver->connect_file_server();
}

void data_manager::terminate_connect() {
	m_file_receiver->Close();

#ifdef RUN_AS_CLIENT
	Logout(&m_proxy_id);
#endif
	Stop();
}

#ifdef RUN_AS_CLIENT
void data_manager::OnLoginOk(E15_ClientInfo * user,E15_String *& json) {
	print_thread_safe(g_log, "[%x:%x] (N=%d, name=%s:role=%s) 上线\n", user->id.h, user->id.l,
			user->name, user->role);
	m_proxy_id = user->id;
}

int data_manager::OnLogout(E15_ClientInfo * user) {
	print_thread_safe(g_log, "[%x:%x] (N=%d, name=%s:role=%s) 下线\n", user->id.h, user->id.l,
			user->name, user->role);
	return 1000;		//自动重联
}

void data_manager::OnNotify(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json) {
	m_data_mgr->data_dispatch(cmd->cmd, json);
}
#else
int data_manager::OnOpen(E15_ServerInfo * info,E15_String *& json) {
	print_thread_safe(g_log, "[%x:%x] (N=%d, name=%s:role=%s) 上线\n", info->id.h, info->id.l,
			info->N, info->name, info->role);
	if (info->N == 0)
		m_client_id = info->id;

	if (!strcmp(info->role, MARKET_DATA_NODE) ||		//行情服务器
			!strcmp(info->role, TRADE_SERVER_NODE) ||	//交易服务器
			!strcmp(info->role, CLIENT_UI_NODE) ||			//前端界面
			!strcmp(info->role, OPE_MANAGER_NODE))		//运维管理
		m_role_id[info->role] = info->id;
	return 0;
}

int data_manager::OnClose(E15_ServerInfo * info) {
	print_thread_safe(g_log, "[%x:%x] (N=%d, name=%s:role=%s) 下线\n", info->id.h, info->id.l,
			info->N, info->name, info->role);

	m_role_id.erase(info->role);
	return 1000;		//自动重联
}

void data_manager::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
	if (m_role_id[OPE_MANAGER_NODE] == info->id) {		//接到的是后台运维节点的命令，由运维控制策略的加载及卸载
		switch (cmd->cmd) {
		case OPERA_LOAD: {
			//总是从文件服务器中取so，保证每次执行的都是最新的
			m_file_receiver->request_file_by_url(data->c_str());		//so
			break;
		}
		case OPERA_UNLOAD: {
//			auto args = crx::split(data->c_str(), " ");
//			std::string so = STRATEGY_DIR+args[0].substr(args[0].rfind("/")+1);
//			std::string ini = STRATEGY_DIR+args[1].substr(args[1].rfind("/")+1);
//			m_data_mgr->unload_strategy(std::vector<std::string>({so, ini}));
			break;
		}
		default:
			break;
		}
	}
}

void data_manager::OnNotify(E15_ServerInfo * info,
		E15_ServerRoute * rt,
		E15_ServerCmd * cmd,
		E15_String *& data) {
	if (Stock_Msg_DiagramInfo == cmd->cmd)
		m_dia_id = info->id;
	m_data_mgr->data_dispatch(cmd->cmd, data);
}
#endif

void data_manager::load_strategy(const std::vector<std::string>& args) {
	m_data_mgr->load_strategy(args);
}

int data_manager::request_subscribe_by_id(E15_StringArray& sa, int start, int end, int interval) {
	/* Construct Json string according to the `instrument_name` */
	E15_ValueTable vt;
	vt.InsertS("id_list")->SetStringArray(&sa);
	vt.SetSI("start_date", start);
	vt.SetSI("end_date", end);
	vt.SetSI("interval", interval);
	vt.InsertS("conn_real")->SetUInt(g_conf.conn_real);

	E15_String s;
	vt.Dump(&s);


#ifdef RUN_AS_CLIENT
	E15_ClientMsg msg;
	msg.cmd = Stock_Msg_SubscribeById;
	return Request(&m_proxy_id, &msg, s.c_str(), s.Length());
#else
	E15_ServerCmd cmd;
	cmd.cmd = Stock_Msg_SubscribeById;
	return Request(&m_dia_id, 0, &cmd, &s);
#endif
}

int data_manager::request_unsubscribe_by_id(E15_StringArray& sa) {
	E15_ValueTable vt;
	vt.InsertS("id_list")->SetStringArray(&sa);
	vt.InsertS("conn_real")->SetUInt(g_conf.conn_real);

	E15_String s;
	vt.Dump(&s);

#ifdef RUN_AS_CLIENT
	E15_ClientMsg msg;
	msg.cmd = Stock_Msg_UnSubscribeById;
	return Request(&m_proxy_id, &msg, s.c_str(), s.Length());
#else
	E15_ServerCmd cmd;
	cmd.cmd = Stock_Msg_UnSubscribeById;
	return Request(&m_dia_id, 0, &cmd, &s);
#endif
}

int data_manager::request_subscribe_all() {
	E15_ValueTable vt;
	vt.InsertS("conn_real")->SetUInt(g_conf.conn_real);

	E15_String s;
	vt.Dump(&s);

#ifdef RUN_AS_CLIENT
	E15_ClientMsg msg;
	msg.cmd = Stock_Msg_SubscribeAll;
	return Request(&m_proxy_id, &msg, s.c_str(), s.Length());
#else
	E15_ServerCmd cmd;
	cmd.cmd = Stock_Msg_SubscribeAll;
	return Request(&m_dia_id, 0, &cmd, &s);
#endif
}

int data_manager::request_unsubscribe_all() {
	E15_ValueTable vt;
	vt.InsertS("conn_real")->SetUInt(g_conf.conn_real);

	E15_String s;
	vt.Dump(&s);

#ifdef RUN_AS_CLIENT
	E15_ClientMsg msg;
	msg.cmd = Stock_Msg_UnSubscribeAll;
	return Request(&m_proxy_id, &msg, s.c_str(), s.Length());
#else
	E15_ServerCmd cmd;
	cmd.cmd = Stock_Msg_UnSubscribeAll;
	return Request(&m_dia_id, 0, &cmd, 0);
#endif
}

void data_manager::send_instruction(const std::string& ins_id, const order_instruction& oi) {
	E15_String s_trade, s_ui;
	s_ui.Memcpy(ins_id.c_str(), ins_id.size());
	s_ui.Resize(16, 0);
	s_ui.Memcat((const char*)&oi, sizeof(order_instruction));

	TradeTaskRequest req;
	memset(&req, 0, sizeof(req));
	memcpy(req.Instrument, ins_id.c_str(), ins_id.size());
	req.req_id.date = oi.current.date;
	req.req_id.time = oi.current.time;
	req.req_id.strategy_id = oi.strategy_id;
	req.req_id.src = oi.src;
	req.req_id.seq = oi.trade_seq;
	req.price = oi.price*10000;
	req.volume = oi.vol_cnt;
	if (DIRECTION_BUY == oi.direction)
		req.Direct = 1;
	else if (DIRECTION_SELL == oi.direction)
		req.Direct = -1;
	if (FLAG_OPEN == oi.flag)
		req.open_close = 1;
	else if (FLAG_CLOSE == oi.flag)
		req.open_close = 0;
	req.level = oi.level;
	s_trade.Memcpy((const char*)&req, sizeof(TradeTaskRequest));

#ifdef RUN_AS_CLIENT
	E15_ClientMsg msg;
	msg.cmd = TRADE_MSG_INPUT_ORDER;
	Request(&m_proxy_id, &msg, s.c_str(), s.Length());
#else
	E15_ServerCmd cmd;
	cmd.cmd = TRADE_MSG_INPUT_ORDER;
	//给交易服务器发送请求交易的指令，不允许丢包，使用Request发送请求
	Request(&m_role_id[TRADE_SERVER_NODE], 0, &cmd, &s_trade);
	//同时将交易指令推给前端界面，若系统无法及时处理，允许丢包
	Notify(&m_client_id, 0, &cmd, &s_ui);
#endif
}
