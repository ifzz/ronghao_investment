#pragma once

#include "rhafx.h"

struct trade_config {
	std::string dbname;		//数据库名称
	std::string pyscript_name;		//脚本名称
	std::string init_func;		//脚本初始函数
	std::string destroy_func;		//脚本销毁函数
	std::string db_rootdir;	//数据库导入日志的根目录
};

class trade_access;
class data_trans : public E15_Server {
public:
	data_trans(trade_access *accessor) : m_accessor(accessor) {}
	virtual ~data_trans() {}

public:
	void start();
	void stop();
	void send_orderins(E15_Id& node_id, const std::string& data);

public:
	/* server callback */
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json);
	virtual int OnClose(E15_ServerInfo * info);
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}

private:
	trade_access *m_accessor;
	E15_ValueTable m_vt;
};

class trade_access : public ss_util {
public:
	trade_access();
	virtual ~trade_access() {}

	virtual bool init(bool is_service, int argc, char *argv[]);
	virtual void destroy();

	void handle_history_req(E15_Id& node_id, const std::string& ins_id, long stg_id,
			long long begin_dt, long long end_dt);

private:
	void handle_orderins_req(const std::string& ins_id, long stg_id, long long begin_dt, long long end_dt);
	void handle_logfile_req(const std::string& ins_id, long stg_id, unsigned int today,
			long long begin_dt, long long end_dt);

private:
//	std::thread m_dbimp_th;
//	crx::py_wrapper m_py_wrap;
	std::shared_ptr<data_trans> m_data_trans;

	sqlite3 *m_sqlite_db;
	char *m_err_msg, m_sql[256];
	std::string m_sql_result;

	std::map<std::string, std::string> m_row;
	order_instruction m_oi;
	wd_seq m_begin, m_end;
};
