#pragma once

#include "data_trans.h"
#include "store_history.h"

extern void print_thread_safe(const char *format, ...);

class history_mgr;
class data_parser : public crx::evd_thread_processor {
public:
	data_parser(const std::string& ins_id, unsigned int start, unsigned int end, history_mgr *ptr);
	virtual ~data_parser() {}

public:
	virtual void process_task(std::shared_ptr<crx::evd_thread_job> job);
	static int get_depth(void * obj,int market,const char * id,MarketDepthBase * base,MarketDepthExt * ext,MarketBidAsk * ba);

private:
	std::string m_ins_id;
	unsigned int m_current_date;
	unsigned int m_start, m_end;
	history_mgr *m_mgr_ptr;
	std::deque<std::shared_ptr<MarketDepthData>> m_depth_list;
};

struct timer_args {
	history_mgr *mgr_ptr;
	int type;			//job type
	uint64_t cnt;
	std::shared_ptr<data_parser> parser;

	timer_args(history_mgr *ptr, int i)
	:mgr_ptr(ptr)
	,type(i)
	,cnt(0) {}
};

struct ins_info {
	int sub_cnt;		//subscribe count
	int fd;		//for timer task
	std::shared_ptr<timer_args> ta;

	ins_info()
	:sub_cnt(-1)
	,fd(-1) {}
};

/**
 * 历史行情服务器从服务器上取出指定合约的指定日期的所有tick级数据，并将这些数据转发给实时行情服务器，
 * 由实时行情根据tick级数据加工出指标数据之后统一将整个group转发给策略服务器
 */
class history_mgr : public crx::console {
public:
	history_mgr() { m_trans = std::make_shared<data_trans>(this); }
	virtual ~history_mgr() {}

public:
	virtual bool init(int argc, char *argv[]);
	virtual void destroy();
	void send_history(const std::string& ins_id, E15_String *data, int cmd) {
		m_trans->send_data(ins_id, data, cmd);
	}
	void notify_over(const std::string& ins_id);
	void load_depth(const char *ins_id, data_parser *parser, unsigned int date);
	void history_subscribe(const char *id, unsigned int start, unsigned int end, unsigned int millisec);
	void history_unsubscribe(const char *id);

private:
	static void timer_callback(int fd, void *args);

private:
	crx::epoll_thread m_timer_th;
	crx::evd_thread_pool m_thread_pool;

	std::vector<char> m_bitset;
	std::map<std::string, ins_info> m_ins_info;		//it->first: ins_id, it->second: ins_info

	std::shared_ptr<data_trans> m_trans;
	E15_HistoryStore *m_history_store;
};
