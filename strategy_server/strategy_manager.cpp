#include "stdafx.h"

E15_Log g_log;		//整个策略框架共用同一个日志

E15_Socket g_socket;
static strategy_manager g_mgr;

config g_conf;

processor::processor(strategy_manager *mgr_ptr)
:m_handle(nullptr)
,m_spid(-1)
,m_mgr_ptr(mgr_ptr)
,m_rd_fifo(-1)
,m_wr_fifo(-1) {}

void processor::process_task(std::shared_ptr<crx::evd_thread_job> job) {
	auto j = std::dynamic_pointer_cast<strategy_job>(job);
	if (g_conf.for_produce) {		//生产环境
		m_strategy->execute(j->group);
	} else {		//测试环境
		int32_t cmd = Stock_Msg_DiagramGroup;
		m_seria.insert("cmd", (const char*)&cmd, sizeof(cmd));
		m_seria.insert("data", j->raw->c_str(), j->raw->Length());
		m_seria.write(m_rd_fifo);
		m_seria.reset();
	}
}

bool processor::load_share_for_produce(void *handle, const std::string& l, const std::string& c) {
	strategy_export create_strategy = (strategy_export)(dlsym(handle, "create_strategy"));
	if (!create_strategy) {
		print_thread_safe(g_log, "[strategy manager] Load function 'create_strategy' failed with error: %s, please make sure the "
				"implementation of strategy '%s' comply with the established specifications!\n", dlerror(), l.c_str());
		return false;
	}

	m_strategy = create_strategy();
	sb_impl *impl = static_cast<sb_impl*>(m_strategy->m_obj);
	impl->m_mgr_ptr = m_mgr_ptr;
	m_strategy->read_config(c.c_str());		//读配置
	m_strategy->init();		//初始化
	print_thread_safe(g_log, "[strategy manager] create strategy instance with "
			"library name `%s` successfully!\n", l.c_str());
	return true;
}

bool processor::create_child_for_test(const std::string& l, const std::string& c,
		E15_String *ins_list, E15_String *diagram_info) {
	//创建读写管道，写管道用于行情数据的分发，读管道用于转发请求交易的指令
	std::string uid = l;		//构造唯一标识，用于区分各个子进程之间的fifo
	std::string::size_type start_pos = uid.rfind("/"), end_pos = uid.rfind(".");
	uid = uid.substr(start_pos+1, end_pos-start_pos-1);

	std::string temp = c;
	start_pos = temp.rfind("/"), end_pos = temp.rfind(".");
	uid += temp.substr(start_pos+1, end_pos-start_pos-1);

	char fifo_file[256] = {0};
	sprintf(fifo_file, "%s%s%d", FIFO_PREFIX, uid.c_str(), STDIN_FILENO);		//读管道
	if (!access(fifo_file, F_OK))
		remove(fifo_file);		//若管道存在，则先删除
	mkfifo(fifo_file, 0777);
	m_rfifo = fifo_file;

	sprintf(fifo_file, "%s%s%d", FIFO_PREFIX, uid.c_str(), STDOUT_FILENO);		//写管道
	if (!access(fifo_file, F_OK))
		remove(fifo_file);
	mkfifo(fifo_file, 0777);
	m_wfifo = fifo_file;

	pid_t pid = fork();
	if (pid < 0) {
		print_thread_safe(g_log, "error in fork!\n");
		return false;
	} else if (0 == pid) {		//child process
		//first arg is the executable file, the followed is the argv[0] in that file
		execl("./test_child", "./test_child", uid.c_str(), l.c_str(), c.c_str(), (const char*)0);
	} else {		//parent process
		m_spid = pid;
		m_rd_fifo = open(m_rfifo.c_str(), O_RDONLY);
		m_wr_fifo = open(m_wfifo.c_str(), O_WRONLY);

		//发送合约列表和指标描述信息
		int32_t cmd = Stock_Msg_InstrumentList;
		m_seria.insert("cmd", (const char*)&cmd, sizeof(cmd));
		m_seria.insert("data", ins_list->c_str(), ins_list->Length());
		m_seria.write(m_wr_fifo, crx::COMP_ZIP);
		m_seria.reset();

		cmd = Stock_Msg_DiagramInfo;
		m_seria.insert("cmd", (const char*)&cmd, sizeof(cmd));
		m_seria.insert("data", diagram_info->c_str(), diagram_info->Length());
		m_seria.write(m_wr_fifo, crx::COMP_ZIP);
		m_seria.reset();
		print_thread_safe(g_log, "父进程(%d)发送合约列表(bytes=%d)和指标描述信息(bytes=%d)完毕！\n",
				getpid(), ins_list->Length(), diagram_info->Length());
	}
	return true;
}

void processor::destroy_child_for_test(const std::string& l, const std::string& c) {
	long sig_quit = -1;		//发送子进程退出信号
	m_seria.insert("cmd", (const char*)&sig_quit, sizeof(sig_quit));
	m_seria.write(m_wr_fifo);
	m_seria.reset();

	close(m_rd_fifo);
	close(m_wr_fifo);
	remove(m_rfifo.c_str());
	remove(m_wfifo.c_str());
}

void strategy_manager::parse_ini() {
	E15_Ini ini;
	ini.Read("ini/config.ini");
	ini.SetSection("client");
	g_conf.ip = ini.ReadString("addr", "127.0.0.1");
	ini.Read("port", g_conf.port);
	g_conf.user = ini.ReadString("user", "test");
	g_conf.passwd = ini.ReadString("password", "123456");

	ini.SetSection("setting");
	ini.Read("conn_real", g_conf.conn_real);
	ini.Read("for_produce", g_conf.for_produce);
}

void strategy_manager::child_crash(pid_t pid) {
	for (auto& l : m_libraries) {
		for (auto& c : l.second.ini_map) {
			if (c.second->get_pid() == pid) {
				std::vector<std::string> args = {l.first, c.first};
				unload_strategy(args);
			}
		}
	}
}

void sig_child(int sig_no) {
	int sts = 0, pid;
	while ((pid = waitpid(-1, &sts, WNOHANG)) > 0) {
		g_mgr.child_crash(pid);
		print_thread_safe(g_log, "收到子进程退出信号，回收子进程 pid = %d, sts = %d\n", pid, sts);
	}
}

bool strategy_manager::init(int argc, char *argv[]) {
	if (access(STRATEGY_DIR, F_OK))
		mkdir(STRATEGY_DIR, 0755);
	if (access(DATABASE_IMPORT, F_OK))		//创建数据导入目录
		mkdir(DATABASE_IMPORT, 0755);
	if (access(FIFO_PREFIX, F_OK))
		mkdir(FIFO_PREFIX, 0755);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, [](int sig_no)->void {
		sig_child(sig_no);
	});

	parse_ini();
	global_log_init();
	g_log.Init("strategy_manager", 100);
	g_socket.Start();

	m_threads.start();
	if (!g_conf.for_produce)		//在测试环境中需要接收子进程发送的交易数据，单独开辟一个线程接收
		m_trade_th.start();
	m_data_recv->connect_data_server();
	m_statis_th = std::thread(statis_thread, &m_statis_stop);
	return true;
}

void strategy_manager::destroy() {
	//首先卸载所有的策略
	std::set<std::vector<std::string>> args_set;
	for (auto& l : m_libraries) {
		for (auto& c : l.second.ini_map) {
			std::vector<std::string> args = {l.first, c.first};
			args_set.insert(args);
		}
	}
	for (auto& args : args_set)
		unload_strategy(args);

	m_statis_stop = true;
	m_statis_th.join();
	m_data_recv->terminate_connect();
	if (!g_conf.for_produce)		//回收接收交易数据的线
		m_trade_th.stop();
	m_threads.stop();

	g_socket.Stop();
	print_thread_safe(g_log, "[strategy_manager::destroy]中断与行情服务器的连接，释放服务申请的资源\n");
	global_log_destroy();
}

void strategy_manager::data_dispatch(int cmd, E15_String *&data) {
	switch (cmd) {
	case Stock_Msg_InstrumentList: {
		parse_instrument_list(data->c_str(), data->Length());
		m_ins_list = data;
		data = nullptr;		//接管数据
		printf("strategy_manager[%d] 收到合约列表 bytes=%ld\n", getpid(), m_ins_list->Length());
		break;
	}

	case Stock_Msg_DiagramInfo: {			//diagram description info
		parse_diagram_info(data->c_str(), data->Length());
		m_diagram_info = data;
		data = nullptr;
		printf("strategy_manager[%d] 收到收到指标描述信息 bytes=%ld\n", getpid(), m_diagram_info->Length());

		std::vector<std::string> args = {STRATEGY_DIR"libstrategy_demo3.so", STRATEGY_DIR"libstrategy_demo3.ini"};
		load_strategy(args);
		break;
	}

	case Stock_Msg_DiagramGroup: {
		if (data->Length() < DEPTH_MARKET_HEAD_LEN)
			return;

		std::shared_ptr<strategy_job> job = std::make_shared<strategy_job>();
		if (g_conf.for_produce) {
			job->group = parse_diagram_group(data->c_str(), data->Length());
		} else {
			job->raw = data;
			data = nullptr;
		}
		m_threads.job_dispatch(job);
		break;
	}
	}
}

void strategy_manager::register_rfifo(int fd) {
	if (m_fd_des.end() != m_fd_des.find(fd))
		return;

	m_fd_des[fd] = std::make_shared<crx::deseria>();
	m_fd_des[fd]->set_rfd(fd);
	m_trade_th.add_epoll_event(fd, for_trade, this);
}

void strategy_manager::unregister_rfifo(int fd) {
	if (m_fd_des.end() == m_fd_des.find(fd))
		return;

	m_trade_th.remove_epoll_event(fd);
	m_fd_des.erase(fd);
}

void strategy_manager::for_trade(int fd, void *args) {
	strategy_manager *this_ptr = (strategy_manager*)args;
	int ret = this_ptr->m_fd_des[fd]->read();
	if (1 == ret || -1 == ret) {
		this_ptr->unregister_rfifo(fd);
		print_thread_safe(g_log, "读子进程发送的请求交易指令失败 ret = %d\n", ret);
	}

	this_ptr->m_fd_des[fd]->for_each_map([&](std::map<std::string, std::string>& m, void *args)->void {
		order_instruction *oi = (order_instruction*)m["oi"].data();
		this_ptr->send_instruction(m["ins_id"], *oi);
	}, nullptr);
}

void strategy_manager::statis_thread(bool *want_to_stop) {
	E15_Log statis_log;
	statis_log.Init("tech_statis", 100);

	crx::statis statis(getpid());
	float mem_occupy_peak = 0, cpu_occupy_peak = 0;

	int cnt = 0;
	while (!*want_to_stop) {
		if (!cnt) {
			crx::process_mem_occupy_t pmem = statis.get_process_mem_occupy();		//内存统计
			crx::sys_mem_dist_t smem = statis.get_sys_mem_dist();
			float mem_occupy = pmem.vm_rss*1.0/smem.mem_total;
			if (mem_occupy > mem_occupy_peak)
				mem_occupy_peak = mem_occupy;

			float cpu_occupy = statis.get_process_cpu_occupy(TEST_INTERVAL);		//cpu统计
			if (cpu_occupy > cpu_occupy_peak)
				cpu_occupy_peak = cpu_occupy;

			int open_file = statis.get_open_file_handle();		//文件句柄数

			int32_t disk_usage = statis.get_disk_usage(".");		//当前目录占用的磁盘量

			statis_log.Printf(0, "\n当前策略服务的内存使用情况统计如下："
					"\n虚拟内存的峰值（VmPeak）：\t%.2f MB "
					"\n虚拟内存的大小（VmSize）：\t%.2f MB"
					"\n物理内存的峰值（VmHWM）：\t%.2f MB"
					"\n物理内存的大小（VmRSS）：\t%.2f MB"
					"\n物理内存的占用量：\t%.2f%%"
					"\n物理内存占用量的峰值：\t%.2f%%"
					"\nCPU的占用量：\t%.2f%%"
					"\nCPU占用量的峰值：\t%.2f%%"
					"\n打开的文件句柄数：\t%d"
					"\n当前进程的磁盘占用量：\t%.2f GB\n\n",
					pmem.vm_peak/1024.0, pmem.vm_size/1024.0, pmem.vm_hwm/1024.0, pmem.vm_rss/1024.0,
					mem_occupy*100, mem_occupy_peak*100, cpu_occupy*100, cpu_occupy_peak*100,
					open_file, disk_usage/1024.0/1024.0/1024.0);
			cnt++;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
		cnt = (cnt+1)%15;		//每隔15s统计一次
	}
}

void strategy_manager::load_strategy(const std::vector<std::string>& args) {
	if (args.size() != 2)
		return;

	auto& l = args[0];
	auto& c = args[1];

	if ("so" != l.substr(l.rfind(".")+1) || "ini" != c.substr(c.rfind(".")+1))
		return;

	if (m_libraries.end() != m_libraries.find(l) &&
			m_libraries[l].ini_map.end() != m_libraries[l].ini_map.find(c))
		return;		//指定的库以及配置文件都存在

	if (m_libraries.end() == m_libraries.find(l)) {
		void *handle = nullptr;
		if (g_conf.for_produce) {
			handle = dlopen(l.c_str(), RTLD_NOW);
			if (!handle) {
				perror("load_strategy::dlopen");
				return;
			}
		}
		m_libraries[l] = library_info();
		m_libraries[l].handle = handle;
	}

	bool load_succ = false;
	std::shared_ptr<processor> p = std::make_shared<processor>(this);
	if (g_conf.for_produce) {		//生产环境
		load_succ = p->load_share_for_produce(m_libraries[l].handle, l, c);
	} else {		//测试环境
		load_succ = p->create_child_for_test(l, c, m_ins_list, m_diagram_info);
	}

	if (!load_succ) {
		if (!m_libraries[l].ini_map.size()) {
			if (m_libraries[l].handle)
				dlclose(m_libraries[l].handle);
			m_libraries.erase(l);
		}
		return;
	}
	m_libraries[l].ini_map[c] = p;
	m_threads.register_processor(p);

	E15_Ini ini;		//在配置中存在关注的合约，若还没订阅则需要订阅
	ini.Read(c.c_str());
	ini.SetSection("framework");
	int start = -1, end = -1, interval = 500;
	ini.Read("start_date", start);
	ini.Read("end_date", end);
	ini.Read("interval", interval);

	ini.SetSection("share");
	const char *ins_id = ini.ReadString("ins_id", "");
	auto ins_vec = crx::split(ins_id, ";");
	E15_StringArray sa;
	for (auto& ins : ins_vec) {
		if (!m_ins_info[ins].subscribe_cnt)
			sa.Add(ins.data(), ins.size());
		m_ins_info[ins].subscribe_cnt++;
	}
	if (sa.Size())
		m_data_recv->request_subscribe_by_id(sa, start, end, interval);
	p->store_sub_ins(ins_vec);
}

void strategy_manager::unload_strategy(const std::vector<std::string>& args) {
	if (args.size() != 2)
		return;

	auto& l = args[0];
	auto& c = args[1];

	if (m_libraries.end() == m_libraries.find(l) ||
			m_libraries[l].ini_map.end() == m_libraries[l].ini_map.find(c))
		return;

	auto pro = std::move(m_libraries[l].ini_map[c]);
	m_threads.unregister_processor(pro);
	if (!g_conf.for_produce)		//测试环境
		pro->destroy_child_for_test(l, c);

	m_libraries[l].ini_map.erase(c);
	if (!m_libraries[l].ini_map.size()) {
		if (g_conf.for_produce)
			pro->unload_so(m_libraries[l].handle);
		m_libraries.erase(l);
	}

	E15_StringArray sa;
	pro->for_each_ins([&](const std::string& ins, void *args)->void {
		m_ins_info[ins].subscribe_cnt--;
		if (!m_ins_info[ins].subscribe_cnt)
			sa.Add(ins.data(), ins.size());
	}, nullptr);
	if (sa.Size())
		m_data_recv->request_unsubscribe_by_id(sa);
	print_thread_safe(g_log, "[strategy manager] unload strategy with library name `%s` successfully!\n", l.c_str());
}

void strategy_manager::show_strategy() {
	int cnt = 1;
	std::cout<<std::setw(8)<<"id"<<std::setw(48)<<"so"<<"ini"<<std::endl;
	for (auto& l : m_libraries)
		for (auto& c : l.second.ini_map)
			std::cout<<std::setw(8)<<cnt++<<std::setw(48)<<l.first<<c.first<<std::endl;
	std::cout<<std::endl;
}

int main(int argc, char *argv[]) {
	g_mgr.add_cmd("load", "l", [](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.load_strategy(args);
	}, "load specified library@usage: load(l) `library` `config`");

	g_mgr.add_cmd("unload", "ul", [](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.unload_strategy(args);
	}, "unload specified library@usage: unload(ul) `strategy` `config`");

	g_mgr.add_cmd("show", "s", [](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.show_strategy();
	}, "show strategies");
	return g_mgr.run(argc, argv);
}
