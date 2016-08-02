#pragma once

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
	:m_threads(THREADS_NUM)
	,m_statis_stop(false) {
		m_data_recv = new data_manager(this);
	}
	virtual ~strategy_manager() { delete m_data_recv; }

public:
	void load_strategy(const std::vector<std::string>& args);
	void unload_strategy(const std::vector<std::string>& args);
	void data_dispatch(int cmd, E15_String *&data);

	virtual bool init(int argc, char *argv[]);
	virtual void destroy();
	virtual void send_instruction(const std::string& ins_id, order_instruction& oi) {
		m_data_recv->send_instruction(ins_id, oi);
	}

	void child_crash(pid_t pid);
	void show_strategy();
	void register_rfifo(int fd);
	void unregister_rfifo(int fd);

private:
	void parse_ini();
	void handle_ins_sub(std::shared_ptr<processor>& p, const std::string& c);
	static void for_trade(int fd, void *args);
	static void statis_thread(bool *want_to_stop);

private:
	data_manager *m_data_recv;
	crx::epoll_thread m_trade_th;
	crx::evd_thread_pool m_threads;
	std::map<std::string, library_info> m_libraries;	//it->first: library_name with path, it->second: library info

	//用于指标测试的成员变量
	bool m_statis_stop;
	std::thread m_statis_th;
	std::map<int, std::shared_ptr<crx::deseria>> m_fd_des;
};
