#pragma once

class log_job : public crx::evd_thread_job {
public:
	log_job(const char *str, E15_Log *l)
	:crx::evd_thread_job(1)
	,print_str(str)
	,log(l) {}

	std::string print_str;
	E15_Log *log;
};

class log_handle : public crx::evd_thread_processor {
public:
	virtual void process_task(std::shared_ptr<crx::evd_thread_job> job);
};

class ss_util;
class sb_impl {
public:
	sb_impl(strategy_base *base)
	:m_stg_id(0)
	,m_src_id(0)
	,m_seq_id(0)
	,m_mgr_ptr(nullptr)
	,m_this_stg(base)
	,m_current_date(0)
	,m_trade_import(nullptr) {}
	virtual ~sb_impl() {}

	void read_config(const char *config);
	void on_init();

	unsigned int m_stg_id;			//策略id
	unsigned short m_src_id;		//下单源id
	unsigned short m_seq_id;	//流水号
	ss_util *m_mgr_ptr;
	strategy_base *m_this_stg;
	unsigned int m_current_date;
	FILE *m_trade_import;
	char format_buf[1024];
	E15_Log m_log;			//每个具体的策略都有一个私有的日志
};

struct contract_info {
	int64_t type;
	size_t subscribe_cnt;
	ContractInfo detail;

	contract_info()
	:type(-1)
	,subscribe_cnt(0) {}
};

extern crx::evd_thread_pool g_log_th;
extern std::shared_ptr<log_handle> g_log_hd;

class ss_util : public crx::console {
public:
	ss_util();
	virtual ~ss_util();

	virtual bool init(int argc, char *argv[]) { return true; }
	virtual void destroy() {}

	ContractInfo get_ins_info(const std::string& id);
	void for_each_ins(std::function<void(const std::string&, const ContractInfo&, void*)> f, void *args);
	datetime get_current_datetime();

	virtual unsigned int get_usable_stg_id() { return 0; }
	virtual void send_instruction(const std::string& ins_id, order_instruction& oi) = 0;

protected:
	void parse_ini();
	void global_log_init();
	void global_log_destroy();

	void parse_instrument_list(const char *data, int len);
	void parse_diagram_info(const char *data, int len);
	depth_dia_group parse_diagram_group(const char *data, int len);

private:
	static int handle_contract_info(E15_Key *key, E15_Value *info, ss_util *mgr_ptr);
	static int handle_diagram_item(E15_Key * key,E15_Value * info,DiagramDataMgr *stock);

protected:
	std::map<std::string, contract_info> m_ins_info;		//it->first: instrument id, it->second: instrument info
	//在测试环境中接收到以下两个信息时保存原始数据，在创建子进程成功之后立即推送这两者
	E15_String *m_ins_list, *m_diagram_info;

	unsigned int m_test_seq;

	E15_Zip m_unzip;
	E15_String m_unzip_buffer;
	E15_ValueTable m_vt;
};
