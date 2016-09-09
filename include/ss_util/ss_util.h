#pragma once

class log_job : public crx::evd_thread_job {
public:
	log_job(const char *str, E15_Log *l, bool ps)
	:crx::evd_thread_job(1)
	,print_str(str)
	,log(l)
	,print_screen(ps) {}

	std::string print_str;
	E15_Log *log;
	bool print_screen;
};

class log_handle : public crx::evd_thread_processor {
public:
	virtual void process_task(std::shared_ptr<crx::evd_thread_job> job);
};

struct cache_info {
	std::map<int, dia_group> cache_dia;
	std::function<void(dia_group&, void*)> f;
	void *args;
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
	void record_trade(const std::string& buf, unsigned int trade_date);

	unsigned int m_stg_id;			//策略id
	unsigned short m_src_id;		//下单源id
	unsigned short m_seq_id;	//流水号
	ss_util *m_mgr_ptr;
	strategy_base *m_this_stg;
	unsigned int m_current_date;
	FILE *m_trade_import;
	char format_buf[1024];
	E15_Log m_log;			//每个具体的策略都有一个私有的日志

	dia_group file_dia;
	std::map<std::string, cache_info> cache;
};

struct contract_info {
	int64_t type;
	size_t subscribe_cnt;
	ContractInfo detail;

	std::map<int, std::list<dia_group>> dia_cache;		////当前合约的图表缓存

	contract_info()
	:type(-1)
	,subscribe_cnt(0) {}
};

extern crx::evd_thread_pool g_log_th;
extern std::shared_ptr<log_handle> g_log_hd;

static int g_stg_id = 0;

class ss_util : public crx::console {
public:
	ss_util();
	virtual ~ss_util();

	virtual bool init(bool is_service, int argc, char *argv[]) { return true; }
	virtual void destroy() {}

	ContractInfo get_ins_info(const std::string& id);
	void for_each_ins(std::function<void(const std::string&, const ContractInfo&, void*)> f, void *args);
	void load_dia_history(const std::string& id, MarketDataType& dt, unsigned int date, unsigned int cdate, void *obj);
	void load_dia_hiscache(const std::string& id, int data_index, DiagramDataHandler *h, short store_level, void *obj);
	void make_index_for_cache();

	virtual unsigned int get_usable_stg_id() { return g_stg_id++; }
	virtual void send_instruction(const std::string& ins_id, order_instruction& oi){}

protected:
	void parse_ini(bool is_service);
	void global_log_init(bool need_history_store);
	void global_log_destroy();

	void parse_instrument_list(const char *data, int len);
	void parse_diagram_info(const char *data, int len);
	depth_dia_group parse_diagram_group(const char *data, int len);
	void parse_cache_diagroup(E15_ServerCmd *cmd, const char *data, int len);

private:
	void load_cache_forstg(const std::string& id, int data_index, DiagramDataItem *item, sb_impl *impl, bool exec);

	static int handle_contract_info(E15_Key *key, E15_Value *info, ss_util *mgr_ptr);
	static int handle_diagram_item(E15_Key * key,E15_Value * info,DiagramDataMgr *stock);

	static int get_dia_history(void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,
			HistoryDiaBase * base,const char * ext_data,int len);

protected:
	std::map<std::string, contract_info> m_ins_info;		//it->first: instrument id, it->second: instrument info
	//在测试环境中接收到以下两个信息时保存原始数据，在创建子进程成功之后立即推送这两者
	E15_String *m_ins_list, *m_diagram_info;

	E15_Zip m_unzip;
	E15_String m_unzip_buffer;
	E15_ValueTable m_vt;
	E15_HistoryStore *m_history_store;
};
