#include "trade_access.h"

E15_Socket g_socket;
trade_config g_trade_conf;

const static int32_t time_base = 1000000000;

void data_trans::start() {
	g_socket.Start();
	Start(&g_socket, "ini/server.ini");
}

void data_trans::stop() {
	Stop();
	g_socket.Stop();
}

void data_trans::send_orderins(E15_Id& node_id, const std::string& data) {
	E15_ServerCmd cmd;
	cmd.cmd = Trade_Msg_ReqHisOrderIns;
	Notify(&node_id, 0, &cmd, data.c_str(), data.size());
}

int data_trans::OnOpen(E15_ServerInfo * info,E15_String *& json) {
	print_thread_safe(g_log, "[%x:%x] (N=%d,name=%s:role=%s) 上线\n", info->id.h,
			info->id.l, info->N, info->name, info->role);
	return 1;
}

int data_trans::OnClose(E15_ServerInfo * info) {
	print_thread_safe(g_log, "[%x:%x] (N=%d, name=%s:role=%s) 下线\n", info->id.h, info->id.l,
			info->N, info->name, info->role);
	return 1000;		//自动重联
}

void data_trans::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
	switch(cmd->cmd) {
	case Trade_Msg_ReqHisOrderIns: {
		//begin_time, end_time, stg_id_list
		std::string ins_id = data->c_str();
		m_vt.Import(data->c_str()+16, data->Length()-16);
		long long begin_dt = m_vt.Int64S("begin_time");
		long long end_dt = m_vt.Int64S("end_time");		//end_time有可能为0，若为0则一直加载到最后
		unsigned long cnt;
		long *stg_array = m_vt.ValueS("stg_id_list")->GetLongArray(&cnt);
		print_thread_safe(g_log, "[OnRequest]begin_time = %lld, end_time = %lld, cnt = %d\n", begin_dt, end_dt, cnt);
		for (size_t i = 0; i < cnt; ++i)
			m_accessor->handle_history_req(info->id, ins_id, stg_array[i], begin_dt, end_dt);
		m_vt.Reset();
		break;
	}
	}
}

trade_access::trade_access()
:m_err_msg(nullptr) {}

bool trade_access::init(bool is_service, int argc, char *argv[]) {
	E15_Ini ini;
	ini.Read("ini/config.ini");
	ini.SetSection("database");
	g_trade_conf.dbname = ini.ReadString("db_name", "");
	g_trade_conf.pyscript_name = ini.ReadString("py_script", "");
	g_trade_conf.db_rootdir = ini.ReadString("dbimpo_dir", "");
	g_trade_conf.init_func = ini.ReadString("py_init_fname", "");
	g_trade_conf.destroy_func = ini.ReadString("py_des_fname", "");
	size_t found = g_trade_conf.pyscript_name.rfind(".");
	if (found != std::string::npos)
		g_trade_conf.pyscript_name = g_trade_conf.pyscript_name.substr(0, found);

	global_log_init(false);
	g_log.Init("trade_access", 1000);
	m_data_trans = std::make_shared<data_trans>(this);
	m_data_trans->start();
//	m_dbimp_th = std::thread([&]()->void {
//		m_py_wrap.run_py_func(m_pyscript_name, m_init_func, 0);
//	});

	//test
//	std::string ins_id = "ni1609";
//	E15_String data;
//	E15_String *s = &data;
//	data.Memcpy(ins_id.c_str(), ins_id.size());
//	data.Resize(16, 0);
//
//	E15_ValueTable vt;
//	vt.SetSInt64("begin_time", 20160905091030000);
//	vt.SetSInt64("end_time", 20160907103030000);
//	long stg_array[2] = {0, 1};
//	vt.InsertS("stg_id_list")->SetLongArray(stg_array, 2);
//	vt.Dump(&data);
//
//	E15_ServerCmd cmd;
//	cmd.cmd = Trade_Msg_ReqHisOrderIns;
//	m_data_trans->OnRequest(0, 0, &cmd, s);
	return true;
}

void trade_access::destroy() {
//	m_py_wrap.run_py_func(m_pyscript_name, m_destroy_func, 0);
//	m_dbimp_th.join();
	m_data_trans->stop();
	global_log_destroy();
}

void trade_access::handle_history_req(E15_Id& node_id, const std::string& ins_id, long stg_id,
		long long begin_dt, long long end_dt) {
	datetime dt = get_current_datetime();
	unsigned int begin_date = begin_dt/time_base;
	unsigned int end_date = end_dt/time_base;
	end_date = (0 == end_date) ? dt.date : end_date;
	if (!begin_dt || begin_date > dt.date || end_date > dt.date)		//begin_time不可能为0，请求日期只可能小于等于当日
		return;

	//构造反馈结果头
	m_sql_result = ins_id;
	m_sql_result.resize(16, 0);
	long long today_tp_begin = begin_dt;

	try {
		if (begin_date < dt.date) {		//需要加载历史买卖点，判断是否有可能同时加载当日买卖点
			long long db_end = (end_date < dt.date) ? end_dt : 0;
			print_thread_safe(g_log, "\n[data_trans::OnRequest]历史买卖点查询请求 ins_id=%s begin_time=%lld db_end=%lld\n",
					ins_id.c_str(), begin_dt, db_end);
			handle_orderins_req(ins_id, stg_id, begin_dt, db_end);

			if (end_date < dt.date)		//只需要加载历史买卖点
				throw std::exception();
			else
				today_tp_begin = 0;
		}

		long long today_tp_end = (0 == end_dt) ? INT64_MAX : end_dt;
		handle_logfile_req(ins_id, stg_id, dt.date, today_tp_begin, today_tp_end);
	} catch (std::exception& e) {}
	m_data_trans->send_orderins(node_id, m_sql_result);		//发送查询结果
}

void trade_access::handle_orderins_req(const std::string& ins_id, long stg_id, long long begin_dt, long long end_dt) {
	sqlite3_open(g_trade_conf.dbname.c_str(), &m_sqlite_db);

	if (end_dt)
		sprintf(m_sql, "SELECT * FROM order_ins WHERE ins_id = '%s' AND stg_id = %ld AND market_datetime BETWEEN "
				"%lld AND %lld;", ins_id.c_str(), stg_id, begin_dt, end_dt);
	else
		sprintf(m_sql, "SELECT * FROM order_ins WHERE ins_id = '%s' AND stg_id = %ld AND market_datetime >= %lld;",
				ins_id.c_str(), stg_id, begin_dt);
	print_thread_safe(g_log, "[handle_orderins_req]sql: %s\n", m_sql);

	sqlite3_exec(m_sqlite_db, m_sql, [](void *arg, int cnt, char *field[], char *col[])->int {
		trade_access *accessor = static_cast<trade_access*>(arg);
		auto& row = accessor->m_row;
		auto& oi = accessor->m_oi;
		bzero(&oi, sizeof(order_instruction));
		for (int i = 0; i < cnt; ++i)
			row[col[i]] = field[i];

		auto& local_dt = row["local_datetime"];
		auto& market_dt = row["market_datetime"];

		oi.uuid.date = atoi(local_dt.substr(0, 8).c_str());
		oi.uuid.time = atoi(local_dt.substr(8).c_str());
		oi.uuid.strategy_id = atoi(row["stg_id"].c_str());
		oi.uuid.src = atoi(row["src_id"].c_str());
		oi.uuid.seq = atoi(row["trade_seq"].c_str());
		auto& dia_name = row["dia_type"];
		memcpy(oi.dia_name, dia_name.c_str(), dia_name.size());
		oi.dia_seq = atoi(row["dia_seq"].c_str());
		if (!strcmp("开仓", row["offset_flag"].c_str()))
			oi.flag = FLAG_OPEN;
		else if (!strcmp("平仓", row["offset_flag"].c_str()))
			oi.flag = FLAG_CLOSE;
		if (!strcmp("买", row["direction"].c_str()))
			oi.direction = DIRECTION_BUY;
		else if (!strcmp("卖", row["direction"].c_str()))
			oi.direction = DIRECTION_SELL;
		oi.price = atoi(row["price"].c_str());
		oi.vol_cnt = atoi(row["vol"].c_str());
		oi.level = atoi(row["level"].c_str());
		oi.market.date = atoi(market_dt.substr(0, 8).c_str());
		oi.market.time = atoi(market_dt.substr(8).c_str());
		print_thread_safe(g_log, "[sqlite]查询到一条记录： market_date=%d market_time=%d\n", oi.market.date, oi.market.time);

		auto& res = accessor->m_sql_result;
		res.append((const char*)&oi, sizeof(order_instruction));
		row.clear();
		return 0;
	}, this, &m_err_msg);

	sqlite3_close(m_sqlite_db);
}

void trade_access::handle_logfile_req(const std::string& ins_id, long stg_id, unsigned int today,
		long long begin_dt, long long end_dt) {
	//构造日志路径
	std::string log_file = g_trade_conf.db_rootdir+std::to_string(today)+"/"+std::to_string(stg_id)+".log";
	if (access(log_file.c_str(), F_OK)) {
		print_thread_safe(g_log, "[handle_logfile_req]无效的日志路径：%s\n", log_file.c_str());
		return;
	}

	std::ifstream fs;
	fs.open(log_file);
	if (!fs.good()) {
		print_thread_safe(g_log, "[handle_logfile_req]打开文件失败 good()=%d\n", fs.good());
		fs.close();
		return;
	}

	std::string line(256, 0);
	while (fs.getline(&line[0], line.size())) {
		bzero(&m_oi, sizeof(order_instruction));
		auto records = crx::split(line, " ");
		//uuid
		auto temp = crx::split(records[0], "#");
		m_oi.uuid.date = atoi(temp[0].substr(6).c_str());
		m_oi.uuid.time = atoi(temp[1].c_str());
		m_oi.uuid.strategy_id = atoi(records[1].c_str());
		m_oi.uuid.src = atoi(records[2].c_str());
		m_oi.uuid.seq = atoi(records[3].c_str());
		std::string store_ins = records[4];
		//diagram
		temp = crx::split(records[5], "#");
		memcpy(m_oi.dia_name, temp[0].c_str(), temp[0].size());
		m_oi.dia_seq = atoi(temp[1].c_str());

		//offset flag & direction
		temp = crx::split(records[6], "#");
		if ("开仓" == temp[0])
			m_oi.flag = FLAG_OPEN;
		else if ("平仓" == temp[0])
			m_oi.flag = FLAG_CLOSE;
		if ("买" == temp[1])
			m_oi.direction = DIRECTION_BUY;
		else if ("卖" == temp[1])
			m_oi.direction = DIRECTION_SELL;

		//price model
		temp = crx::split(records[7], "#");
		m_oi.price = atoi(temp[0].c_str());
		m_oi.vol_cnt = atoi(temp[1].c_str());
		m_oi.level = atoi(temp[2].c_str());

		//market datetime
		temp = crx::split(records[8], "#");
		m_oi.market.date = atoi(temp[0].substr(6).c_str());
		m_oi.market.time = atoi(temp[1].c_str());

		//判断market_dt是否在begin_dt和end_dt之间
		long long market_dt = atoll((temp[0].substr(6)+temp[1]).c_str());
		if (store_ins == ins_id && (begin_dt <= market_dt && market_dt <= end_dt)) {
			print_thread_safe(g_log, "[log]找到一条匹配的记录：market_date = %d, market_time = %d\n",
					m_oi.market.date, m_oi.market.time);
			m_sql_result.append((const char*)&m_oi, sizeof(order_instruction));
		}
	}
	fs.close();
}

int main(int argc, char *argv[]) {
	trade_access accessor;
	return accessor.run(argc, argv);
}
