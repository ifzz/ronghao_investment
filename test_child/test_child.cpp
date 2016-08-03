#include "test_child.h"

static bool g_want_to_stop = false;

bool processor::load(const char *library, const char *config) {
	m_handle = dlopen(library, RTLD_NOW);
	if (!m_handle) {
		printf("[strategy manager(child)] Fail to open shared library '%s' with error: %s\n", library, dlerror());
		return false;
	}

	strategy_export create_strategy = (strategy_export)(dlsym(m_handle, "create_strategy"));
	if (!create_strategy) {
		printf("[strategy manager(child)] Load function 'create_strategy' failed with error: %s, please make sure the "
				"implementation of strategy '%s' comply with the established specifications!\n", dlerror(), library);
		return false;
	}

	m_strategy = create_strategy();
	sb_impl *impl = static_cast<sb_impl*>(m_strategy->m_obj);
	impl->m_mgr_ptr = m_mgr_ptr;
	m_strategy->read_config(config);		//读配置
	m_strategy->init();		//初始化
	printf("[strategy manager] create strategy instance with(run in child %d) library name `%s` with config `%s` succ!\n",
			getpid(), library, config);
	return true;
}

void processor::process_task(std::shared_ptr<crx::evd_thread_job> job) {
//	printf("[strategy_manager in child(%d)]get once group with date=%d, time=%d\n", getpid(),
//			job->group.depth->base.nActionDay, job->group.depth->base.nTime);
	auto j = std::dynamic_pointer_cast<strategy_job>(job);
	m_strategy->execute(j->group);
}

strategy_manager::strategy_manager(int argc, char *argv[]) {
	global_log_init();
	m_eth.start();
	m_thread.start(1);

	m_processor = std::make_shared<processor>(this);
	m_processor->register_type(1);
	m_thread.register_processor(m_processor);

	//使用epoll模型收发数据，所有的fd均设为非阻塞
	char fifo_file[256] = {0};		//argv[1] = uid
	sprintf(fifo_file, "%s%s%d", FIFO_PREFIX, argv[1], STDIN_FILENO);
	m_write_fifo = open(fifo_file, O_WRONLY);

	sprintf(fifo_file, "%s%s%d", FIFO_PREFIX, argv[1], STDOUT_FILENO);
	m_read_fifo = open(fifo_file, O_RDONLY);
	m_deseria.set_rfd(m_read_fifo);
	m_eth.add_epoll_event(m_read_fifo, feed_data, this);

	m_library = argv[2];		//argv[2] = library, argv[3] = config
	m_config = argv[3];
	printf("[strategy_manager (child %d)]成功创建数据处理线程，进入管道(uid=%s)以阻塞方式接收父进程数据...\n",
			getpid(), argv[1]);
}

strategy_manager::~strategy_manager() {
	m_eth.remove_epoll_event(m_read_fifo);
	close(m_read_fifo);
	close(m_write_fifo);

	m_thread.unregister_processor(m_processor);
	m_thread.stop();
	m_eth.stop();
	global_log_destroy();
	printf("[strategy_manager (child %d)] 销毁数据处理线程并退出主循环，释放本进程申请的资源，即将退出...\n", getpid());
}

void strategy_manager::feed_data(int fd, void *arg) {
	strategy_manager *this_ptr = static_cast<strategy_manager*>(arg);
	if (!this_ptr || fd != this_ptr->m_read_fifo)
		return;

	this_ptr->m_deseria.read();		//读取由父进程发送的数据
	this_ptr->m_deseria.for_each_map([&](std::map<std::string, std::string>& m, void *arg)->void {
		int cmd = *(int32_t*)m["cmd"].data();
		if (-1 == cmd) {
			g_want_to_stop = true;
			return;
		}
		this_ptr->data_dispatch(cmd, (const char*)&m["data"].front(), m["data"].size());
	}, nullptr);
}

void strategy_manager::data_dispatch(int cmd, const char *data, size_t len) {
	switch (cmd) {
	case Stock_Msg_InstrumentList: {
		parse_instrument_list(data, len);
		printf("[test_child (%d)] get the instrument list with bytes=%ld succ!\n", getpid(), len);
		break;
	}

	case Stock_Msg_DiagramInfo: {
		parse_diagram_info(data, len);
		printf("[test_child (%d)] get the diagram info with bytes=%ld succ!\n", getpid(), len);
		if (!m_processor->load(m_library, m_config))
			g_want_to_stop = true;
		break;
	}

	case Stock_Msg_DiagramGroup: {
		std::shared_ptr<strategy_job> job = std::make_shared<strategy_job>(1);
		job->group = parse_diagram_group(data, len);
		m_thread.job_dispatch(job);
		break;
	}
	}
}

//在策略执行过程中会触发请求交易的信号，这里的操作只是简单的将交易数据转发给框架
void strategy_manager::send_instruction(const std::string& ins_id, order_instruction& oi) {
	oi.trade_seq = m_seq++;
	m_seria.insert("ins_id", ins_id.c_str(), ins_id.length());
	m_seria.insert("oi", (const char*)&oi, sizeof(order_instruction));
	m_seria.write(m_write_fifo);
	m_seria.reset();
	printf("子进程发送买卖点 ins_id=%s, oi=%p\n", ins_id.c_str(), &oi);
}

int main(int argc, char *argv[]) {
	strategy_manager mgr(argc, argv);
	while (!g_want_to_stop)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return EXIT_SUCCESS;
}
