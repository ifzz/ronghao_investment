#pragma once

#include "data_manager.h"

extern E15_Log g_log;
extern E15_Socket g_socket;

void print_thread_safe(E15_Log& log, const char *format, ...);

class strategy_job : public crx::evd_thread_job {
public:
	strategy_job(int64_t type) : crx::evd_thread_job(type), raw(nullptr) {}
	virtual ~strategy_job() {
		if (raw)
			delete raw;
	}

	depth_dia_group group;
	E15_String *raw;
};

class strategy_manager;
class processor : public crx::evd_thread_processor {
public:
	processor(strategy_manager *mgr_ptr);
	virtual ~processor() {
		m_strategy.reset();
		if (m_handle)
			dlclose(m_handle);
	}

	pid_t get_pid() { return m_spid; }
	bool load_share_for_produce(void *handle, const std::string& l, const std::string& c);
	bool create_child_for_test(const std::string& l, const std::string& c, E15_String *ins_list, E15_String *diagram_info);
	void destroy_child_for_test(const std::string& l, const std::string& c);
	void unload_so(void *handle) { m_handle = handle; }

	void store_sub_ins(std::vector<std::string>& ins_vec) { m_ins_vec = std::move(ins_vec); }
	void for_each_ins(std::function<void(const std::string&, void*)> f, void *args) {
		for (auto& ins : m_ins_vec)
			f(ins, args);
	}

	virtual void process_task(std::shared_ptr<crx::evd_thread_job> job);

private:
	std::string create_pipe(const std::string& l, const std::string& c);

private:
	void *m_handle;
	pid_t m_spid;
	strategy_manager *m_mgr_ptr;
	std::vector<std::string> m_ins_vec;
	std::shared_ptr<strategy_base> m_strategy;		//在生产环境中由策略框架负责加载so并创建策略对象

	crx::seria m_seria;
	int m_rd_fifo, m_wr_fifo;		//读写管道文件描述符
	std::string m_rfifo, m_wfifo;		//读写管道文件名
};

struct library_info {
	std::map<std::string, std::shared_ptr<processor>> ini_map;		//it->first: ini, it->second: strategy info
	void *handle;
	/**
	 * 在生产环境中，是在策略框架中加载卸载so文件，但在测试环境中是在子进程中加载卸载so，因此
	 * 在测试环境中整个框架对指定的策略的具体配置信息是一无所知的，因此也就无法对数据进行过滤，
	 * 在得到数据之后只做简单的转发操作
	 */
	library_info()
	:handle(nullptr) {}
};

class strategy_manager : public ss_util {
public:
	strategy_manager()
	:m_test_exist_data(false)
	,m_statis_stop(false) {
		m_data_recv = new data_manager(this);
	}
	virtual ~strategy_manager() { delete m_data_recv; }
	bool m_test_exist_data;

public:
	virtual bool init(int argc, char *argv[]);
	virtual void destroy();
	virtual unsigned int get_usable_stg_id();
	virtual void send_instruction(const std::string& ins_id, order_instruction& oi) {
		m_data_recv->send_instruction(ins_id, oi);
	}

	void load_strategy(const std::vector<std::string>& args, bool record = true);
	void unload_strategy(const std::vector<std::string>& args, bool record = true);
	void data_dispatch(int cmd, E15_String *&data);

	void child_crash(pid_t pid);
	void flush();
	void show_all();
	void show_run();
	void test(const std::vector<std::string>& args);
	void register_rfifo(int fd);
	void unregister_rfifo(int fd);

private:
	void sub_and_load(bool is_resub);
	void auto_load_stg();
	std::set<std::string> get_stg_dir();
	void print_stg(const std::set<std::string>& dir_set);
	bool parse_stg_dir(const std::string& stg_dir, std::string& l, std::string& c);

	std::shared_ptr<processor> create_processor(const std::string& l, const std::string& c);
	std::shared_ptr<processor> destroy_processor(const std::string& l, const std::string& c);
	void handle_all_sub(std::shared_ptr<processor>& p, const std::string& c);
	void handle_cus_sub(std::shared_ptr<processor>& p, const std::string& c, bool is_resub);
	void handle_ins_unsub(std::shared_ptr<processor>& pro);

	static void for_trade(int fd, void *args);
	static void statis_thread(bool *want_to_stop);

private:
	std::vector<uint8_t> m_stg_runtime_id;		//每个策略的运行时id

	crx::xml_parser m_xml;
	data_manager *m_data_recv;
	crx::epoll_thread m_trade_th;
	crx::evd_thread_pool m_threads;
	std::map<std::string, library_info> m_libraries;	//it->first: library_name with path, it->second: library info

	//用于指标测试的成员变量
	bool m_statis_stop;
	std::thread m_statis_th;
	crx::seria m_seria;
	std::map<int, std::shared_ptr<crx::deseria>> m_fd_des;
};
