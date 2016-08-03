#pragma once
#include "rhafx.h"

class strategy_job : public crx::evd_thread_job {
public:
	strategy_job(int64_t type) : crx::evd_thread_job(type) {}

	depth_dia_group group;
};

class strategy_manager;
class processor : public crx::evd_thread_processor {
public:
	processor(strategy_manager *mgr_ptr) : m_handle(nullptr), m_mgr_ptr(mgr_ptr) {}
	virtual ~processor() {
		m_strategy.reset();
		if (m_handle)
			dlclose(m_handle);
	}

	bool load(const char *library, const char *config);
	virtual void process_task(std::shared_ptr<crx::evd_thread_job> job);

private:
	void *m_handle;
	std::shared_ptr<strategy_base> m_strategy;
	strategy_manager *m_mgr_ptr;
};

class strategy_manager : public ss_util {
public:
	strategy_manager(int argc, char *argv[]);
	virtual ~strategy_manager();

	virtual void send_instruction(const std::string& ins_id, order_instruction& instruction);

private:
	void data_dispatch(int cmd, const char *data, size_t len);
	static void feed_data(int fd, void *arg);

private:
	std::atomic<unsigned short> m_seq;
	crx::seria m_seria;
	crx::deseria m_deseria;
	crx::epoll_thread m_eth;
	crx::evd_thread_pool m_thread;

	int m_read_fifo, m_write_fifo;
	const char *m_library, *m_config;
	std::shared_ptr<processor> m_processor;
};
