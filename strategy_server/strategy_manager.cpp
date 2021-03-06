#include "strategy_manager.h"

static strategy_manager g_mgr;

processor::processor(strategy_manager *mgr_ptr)
:m_handle(nullptr)
,m_spid(-1)
,m_mgr_ptr(mgr_ptr)
,m_rd_fifo(-1)
,m_wr_fifo(-1) {}

void processor::process_task(std::shared_ptr<crx::evd_thread_job> job) {
	auto j = std::dynamic_pointer_cast<strategy_job>(job);
	if (g_conf.for_produce) {		//生产环境
		sb_impl *impl = static_cast<sb_impl*>(m_strategy->m_obj);
		auto& real_group = j->group;
		auto& id = real_group.ins_id;

		//有可能需要cache中的最后一根K线继续初始化
		if (impl->cache.end() != impl->cache.find(id)) {
			//策略中可能会加载多种类型K线的历史
			bool exec = true;
			auto& cache = impl->cache[id];
			for (auto it = cache.cache_dia.begin(); it != cache.cache_dia.end(); ) {
				if (real_group.dias.end() != real_group.dias.find(it->first)) {
					auto& real_dia = real_group.dias[it->first].back();		//取最新的比较
					if (-1 == it->second.data.mode || (real_dia.data.data_base->_date == it->second.data.data_base->_date &&
							real_dia.data.data_base->_seq == it->second.data.data_base->_seq))
						it = cache.cache_dia.erase(it);
					else
						++it;
				} else {
					exec = false;
					break;
				}
			}

			if (exec) {
				for (auto& dia : cache.cache_dia)
					cache.f(dia.second, cache.args);
				impl->cache.erase(id);
			}
		}

		m_strategy->execute(real_group);
	} else {		//测试环境
		int32_t cmd = Stock_Msg_DiagramGroup;
		m_seria.insert("cmd", (const char*)&cmd, sizeof(cmd));
		m_seria.insert("data", j->raw->c_str(), j->raw->Length());
		m_seria.write(m_wr_fifo);
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
	impl->m_stg_id = 0;
	impl->m_src_id = 0;
	impl->read_config(c.c_str());		//读配置
//	impl->on_init();		//初始化
	print_thread_safe(g_log, "[strategy manager] create strategy instance with "
			"library name `%s` successfully!\n", l.c_str());
	return true;
}

std::string processor::create_pipe(const std::string& l, const std::string& c) {
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
	return uid;
}

bool processor::create_child_for_test(const std::string& l, const std::string& c,
		E15_String *ins_list, E15_String *diagram_info) {
	auto uid = create_pipe(l, c);
	pid_t pid = fork();
	if (pid < 0) {
		perror("create_child_for_test::fork");
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

		uint32_t stg_id = 0;
		uint16_t src_id = 0;
		cmd = Stock_Msg_DiagramInfo;
		m_seria.insert("cmd", (const char*)&cmd, sizeof(cmd));
		m_seria.insert("data", diagram_info->c_str(), diagram_info->Length());
		m_seria.insert("stg_id", (const char*)&stg_id, sizeof(uint32_t));
		m_seria.insert("src_id", (const char*)&src_id, sizeof(uint16_t));
		m_seria.write(m_wr_fifo, crx::COMP_ZIP);
		m_seria.reset();
		print_thread_safe(g_log, "父进程(%d)发送合约列表(bytes=%d)和指标描述信息(bytes=%d)完毕！\n",
				getpid(), ins_list->Length(), diagram_info->Length());
		m_mgr_ptr->register_rfifo(m_rd_fifo);
	}
	return true;
}

void processor::destroy_child_for_test(const std::string& l, const std::string& c) {
	long sig_quit = -1;		//发送子进程退出信号
	m_seria.insert("cmd", (const char*)&sig_quit, sizeof(sig_quit));
	m_seria.write(m_wr_fifo);
	m_seria.reset();
	m_mgr_ptr->unregister_rfifo(m_rd_fifo);

	close(m_rd_fifo);
	close(m_wr_fifo);
	remove(m_rfifo.c_str());
	remove(m_wfifo.c_str());
}

void strategy_manager::child_crash(pid_t pid) {
	for (auto& l : m_libraries) {
		for (auto& c : l.second.ini_map) {
			if (c.second->get_pid() == pid) {
				std::vector<std::string> args = {l.first, c.first};
				unload_strategy(args, false);
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

bool strategy_manager::init(bool is_service, int argc, char *argv[]) {
	signal(SIGCHLD, [](int sig_no)->void {
		sig_child(sig_no);
	});

	parse_ini(is_service);
	if (!g_conf.for_produce && access(FIFO_PREFIX, F_OK))
		mkdir(FIFO_PREFIX, 0755);

	m_xml.load("ini/autoload.xml", "config");
	global_log_init(true);
	g_log.Init("strategy_manager", 100);

	m_threads.start(g_conf.threads_num);
	if (!g_conf.for_produce)		//在测试环境中需要接收子进程发送的交易数据，单独开辟一个线程接收
		m_trade_th.start();
	m_data_recv->connect_data_server();
	m_http_gw.Start(&g_socket, g_conf.so_addr.c_str(), g_conf.so_port);
	m_statis_th = std::thread(statis_thread, &m_statis_stop);
	m_so_svr_prefix = "http://"+g_conf.so_addr+"/strategy/";
	m_working_path = crx::get_current_working_path();
	return true;
}

void strategy_manager::destroy() {
	//首先卸载所有的策略
	std::vector<std::string> dir_vec;
	m_xml.for_each_attr([&](const char *name, const char *value, void *args)->void {
		if (atoi(value)) {
			dir_vec.push_back(name);
			unload_strategy(dir_vec, false);
			dir_vec.clear();
		}
	}, nullptr);

	if (g_conf.sub_all)
		m_data_recv->request_unsubscribe_all();

	m_statis_stop = true;
	m_statis_th.join();
	m_http_gw.Stop();
	m_data_recv->terminate_connect();
	if (!g_conf.for_produce)		//回收接收交易数据的线
		m_trade_th.stop();
	print_thread_safe(g_log, "[strategy_manager::destroy]已卸载所有正在执行的策略，并退订所有合约！\n");
	m_threads.stop();
	print_thread_safe(g_log, "[strategy_manager::destroy]中断与行情服务器的连接，释放服务申请的资源\n");

	global_log_destroy();
	remove(FIFO_PREFIX);
}

unsigned int strategy_manager::get_usable_stg_id() {
	unsigned int stg_id;
	auto it = std::find(m_stg_runtime_id.begin(), m_stg_runtime_id.end(), 0);
	if (m_stg_runtime_id.end() != it) {
		*it = 1;
		stg_id = it-m_stg_runtime_id.begin();
	} else {
		stg_id = m_stg_runtime_id.size();
		m_stg_runtime_id.push_back(1);
	}
	return stg_id;
}

void strategy_manager::auto_load_stg() {
	std::vector<std::string> str_vec;
	m_xml.for_each_attr([&](const char *name, const char *value, void *args)->void {
		if (atoi(value)) {
			str_vec.push_back(name);
			load_strategy(str_vec, false);
			str_vec.clear();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}, nullptr);
}

void strategy_manager::sub_and_load(bool is_resub) {
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	if (g_conf.sub_all) {		//使用request_subscribe_by_id接口订阅所有行情，因为需要cache
		E15_StringArray sa;
		for (auto& ins : m_ins_info)
			sa.Add(ins.first.data(), ins.first.size());
		m_data_recv->request_subscribe_by_id(sa, 0, 0, 0);

		if (m_cache_over)		//如果已接收过缓存，那么重复发送的缓存将被过滤，因此这里不需要等待
			return;

		{
			std::unique_lock<std::mutex> lck(m_cache_mtx);
			if (!m_cache_over)		//cache还没完全接收，等待cache完成或cache终止
				m_cache_cv.wait(lck, [&]()->bool { return m_cache_over || m_cache_cancel; });
		}

		if (m_cache_cancel) {
			print_thread_safe(g_log, "与图表服务器断开之后重新连接，终止之前缓存的接收！\n");
			return;
		}
	} else if (is_resub) {
		for (auto& l : m_libraries)
			for (auto& c : l.second.ini_map)
				handle_cus_sub(c.second, c.first, true);
		print_thread_safe(g_log, "[sub_and_load]收到新的指标描述信息，已重新订阅所有策略需要的合约！\n");
	}

	if (!is_resub)		//首次订阅时需要自动加载既定策略
		auto_load_stg();
}

void strategy_manager::notify_cache_over() {
	make_index_for_cache();
	print_thread_safe(g_log, "已处理完成所有合约的缓存数据，开始加载策略！\n");

	{
		std::unique_lock<std::mutex> lck(m_cache_mtx);
		m_cache_over = true;
		m_cache_cv.notify_one();
	}
}

void strategy_manager::data_dispatch(E15_ServerCmd *cmd, E15_String *&data) {
	switch (cmd->cmd) {
	case Stock_Msg_InstrumentList: {
		if (m_ins_list)
			delete m_ins_list;

		parse_instrument_list(data->c_str(), data->Length());
		m_ins_list = data;
		data = nullptr;		//接管数据
		print_thread_safe(g_log, "strategy_manager[%d] 收到合约列表 bytes=%ld\n", getpid(), m_ins_list->Length());
		break;
	}

	case Stock_Msg_DiagramInfo: {			//diagram description info
		int is_resub = 0;
		if (m_diagram_info) {		//重新订阅
			delete m_diagram_info;
			is_resub = 1;
		}

		parse_diagram_info(data->c_str(), data->Length());
		m_diagram_info = data;
		data = nullptr;
		print_thread_safe(g_log, "strategy_manager[%d] 收到收到指标描述信息 bytes=%ld\n", getpid(), m_diagram_info->Length());
		std::string cmd_str = "l __automatic "+std::to_string(is_resub);

		if (is_resub && !m_cache_over) {		//重复订阅且之前的缓存还未接收完毕
			{
				std::unique_lock<std::mutex> lck(m_cache_mtx);
				m_cache_cancel = true;
				m_cache_cv.notify_one();		//唤醒且终止自动加载策略的任务
			}
			DiagramDataMgr_RemoveAll();
		}
		m_cache_cancel = false;
		write_console(cmd_str.data(), cmd_str.size());
		break;
	}

	case Stock_Msg_DiagramCacheData:
	case Stock_Msg_DiagramCacheTag: {
		if (m_cache_over || !g_conf.for_produce)
			return;

		if (data->Length() < DEPTH_MARKET_HEAD_LEN)
			return;

		parse_cache_diagroup(cmd, data->c_str(), data->Length());
		break;
	}

	case Stock_Msg_DiagramGroup: {
		if (data->Length() < DEPTH_MARKET_HEAD_LEN)
			return;

		std::shared_ptr<strategy_job> job = std::make_shared<strategy_job>(m_ins_info[data->c_str()].type);
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
		return;
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

			float cpu_occupy = statis.get_process_cpu_occupy(1000);		//cpu统计
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

void strategy_manager::handle_all_sub(std::shared_ptr<processor>& p, const std::string& c) {
	E15_Ini ini;
	ini.Read(c.c_str());
	int sub_all = 0;
	ini.SetSection("share");
	ini.Read("sub_all", sub_all);		//如果已经设置订阅所有行情，则不再订阅指定合约
	std::vector<std::string> ins_vec;
	if (sub_all) {
		for (auto& ins : m_ins_info) {
			p->register_type(ins.second.type);
			ins_vec.push_back(ins.first);
		}
	} else {
		const char *ins_id = ini.ReadString("ins_id", "");
		ins_vec = crx::split(ins_id, ";");
		for (auto& ins : ins_vec)
			p->register_type(m_ins_info[ins].type);
	}
}

void strategy_manager::handle_cus_sub(std::shared_ptr<processor>& p, const std::string& c, bool is_resub) {
	//在配置中存在关注的合约，若还没订阅则需要订阅
	E15_Ini ini;
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
		if (is_resub) {
			sa.Add(ins.data(), ins.size());
			continue;
		}

		if (!m_ins_info[ins].subscribe_cnt)
			sa.Add(ins.data(), ins.size());
		m_ins_info[ins].subscribe_cnt++;
		p->register_type(m_ins_info[ins].type);
	}

	if (sa.Size())
		m_data_recv->request_subscribe_by_id(sa, start, end, interval);

	p->store_sub_ins(ins_vec);
}

std::shared_ptr<processor> strategy_manager::create_processor(const std::string& l, const std::string& c) {
	if (m_libraries.end() == m_libraries.find(l)) {
		void *handle = nullptr;
		if (g_conf.for_produce) {
			handle = dlopen(l.c_str(), RTLD_NOW);
			if (!handle) {
				perror("load_strategy::dlopen");
				return nullptr;
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
		return nullptr;
	}
	m_libraries[l].ini_map[c] = p;
	return p;
}

void strategy_manager::load_strategy(const std::vector<std::string>& args, bool record /*= true*/) {
	if (args.size() == 2 && args[0] == "__automatic") {
		bool is_resub = atoi(args[1].c_str());
		sub_and_load(is_resub);
		return;
	}

	if (args.size() != 1)
		return;

	std::string l, c;
	if (!parse_stg_dir(g_conf.stg_dir+"/"+args[0], l, c))
		return;

	if (m_libraries.end() != m_libraries.find(l) &&
			m_libraries[l].ini_map.end() != m_libraries[l].ini_map.find(c))
		return;		//指定的库以及配置文件都存在

	auto p = create_processor(l, c);
	if (!p)
		return;
	if (record)
		m_xml.set_attribute(args[0].c_str(), "1", false);

	if (g_conf.sub_all)
		handle_all_sub(p, c);
	else
		handle_cus_sub(p, c, false);

	p->init_strategy();
	m_threads.register_processor(p);
}

std::shared_ptr<processor> strategy_manager::destroy_processor(const std::string& l, const std::string& c) {
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
	print_thread_safe(g_log, "[strategy manager] unload strategy with library name `%s` successfully!\n", l.c_str());
	return pro;
}

void strategy_manager::handle_ins_unsub(std::shared_ptr<processor>& pro) {
	E15_StringArray sa;
	pro->for_each_ins([&](const std::string& ins, void *args)->void {
		m_ins_info[ins].subscribe_cnt--;
		if (!m_ins_info[ins].subscribe_cnt)
			sa.Add(ins.data(), ins.size());
	}, nullptr);
	if (sa.Size())
		m_data_recv->request_unsubscribe_by_id(sa);
}

void strategy_manager::unload_strategy(const std::vector<std::string>& args, bool record /*= true*/) {
	if (args.size() != 1)
		return;

	std::string l, c;
	if (!parse_stg_dir(g_conf.stg_dir+"/"+args[0], l, c))
		return;

	if (m_libraries.end() == m_libraries.find(l) ||
			m_libraries[l].ini_map.end() == m_libraries[l].ini_map.find(c))
		return;

	auto pro = destroy_processor(l, c);
	if (record)
		m_xml.set_attribute(args[0].c_str(), "0", false);

	if (!g_conf.sub_all)
		handle_ins_unsub(pro);
}

bool strategy_manager::parse_stg_dir(const std::string& stg_dir, std::string& l, std::string& c) {
	crx::depth_first_traverse_dir(stg_dir, [&](const std::string& file, void *arg)->void {
		if (".so" == file.substr(file.rfind(".")))
			l = file;
		if (".ini" == file.substr(file.rfind(".")))
			c = file;
	}, nullptr);

	if (l.empty() || c.empty())
		return false;
	return true;
}

std::set<std::string> strategy_manager::get_stg_dir() {
	std::set<std::string> dir_set;
	crx::depth_first_traverse_dir(g_conf.stg_dir, [&](const std::string& file, void *args)->void {
		std::string path = file.substr(file.find("/")+1);
		dir_set.insert(path.substr(0, path.rfind("/")));
	}, nullptr);
	return dir_set;
}

void strategy_manager::flush() {
	auto dir_set = get_stg_dir();
	m_xml.for_each_attr([&](const char *name, const char *value, void *args)->void {
		if (dir_set.end() == dir_set.find(name) && !atoi(value))
			m_xml.delete_attribute(name, false);
	}, nullptr);
	m_xml.flush();
	printf("xml文件更新完成！\n");
}

void strategy_manager::show_all() {
	auto dir_set = get_stg_dir();
	print_stg(dir_set);
}

void strategy_manager::show_run() {
	std::set<std::string> dir_set;
	m_xml.for_each_attr([&](const char *name, const char *value, void *args)->void {
		if (atoi(value))
			dir_set.insert(name);
	}, nullptr);
	print_stg(dir_set);
}

void strategy_manager::print_stg(const std::set<std::string>& dir_set) {
	int cnt = 1;
	std::stringstream ss;
	ss<<'\n'<<std::left<<std::setw(8)<<"seq"<<"stg_ins\n";
	for (auto& dir : dir_set)
		ss<<std::left<<std::setw(8)<<cnt++<<dir<<'\n';
	std::cout<<ss.rdbuf()<<'\n';
	std::cout.flush();
}

void strategy_manager::test(const std::vector<std::string>& args) {
	int sec = atoi(args[0].c_str());
	if (sec <= 0)
		return;

	m_test_exist_data = true;
	std::this_thread::sleep_for(std::chrono::seconds(sec));
	m_test_exist_data = false;
}

void strategy_manager::request_strategy(const std::vector<std::string>& args) {
	if (args.empty())
		return;

	for (auto& stg_id : args) {
		std::string so_file = stg_id+".so", ini_file = stg_id+".ini";
		std::string stg_ins_dir = g_conf.stg_dir+"/"+stg_id+"/";
		if (access(stg_ins_dir.c_str(), F_OK))		//目录不存在则创建
			mkdir(stg_ins_dir.c_str(), 0755);

		//so key
		std::string so_store_file = "../../"+g_conf.so_local_dir+so_file;
		std::string so_sym_file = so_file;
		std::string so_key = "so "+stg_id+" "+so_store_file+" "+so_sym_file+" "+stg_ins_dir+" "+m_working_path;

		//ini key
		std::string ini_store_file = stg_ins_dir+ini_file;
		std::string ini_key = "ini "+stg_id+" "+ini_store_file;

		url_key key;
		key.so_key = so_key;
		key.ini_key = ini_key;
		m_id_key[stg_id] = key;

		std::string so_url = m_so_svr_prefix+stg_id+"/"+so_file;
		std::string ini_url = m_so_svr_prefix+stg_id+"/"+ini_file;
		m_http_gw.Get(m_id_key[stg_id].so_key.c_str(), so_url.c_str(), nullptr, OnResponed);
		m_http_gw.Get(m_id_key[stg_id].ini_key.c_str(), ini_url.c_str(), nullptr, OnResponed);
		print_thread_safe(g_log, "请求文件 id=%s so_url=%s, ini_url=%s\n", stg_id.c_str(), so_url.c_str(), ini_url.c_str());
	}
}

void strategy_manager::OnResponed(const char * key,int status,E15_ValueTable *& header,E15_String *& data) {
	if (200 != status) {
		print_thread_safe(g_log, "文件请求失败 key=%s, status=%d\n", key, status);
		return;
	}

	auto key_vec = crx::split(key, " ");
	if (key_vec[0] == "so") {		//so文件
		auto& stg_id = key_vec[1];
		auto& so_store_file = key_vec[2];
		auto& so_sym_file = key_vec[3];

		chdir(key_vec[4].c_str());
		FILE *fp = fopen(so_store_file.c_str(), "w");
		fwrite(data->c_str(), 1, data->Length(), fp);
		fclose(fp);

		if (!crx::symlink_exist(so_sym_file.c_str()) && -1 == symlink(so_store_file.c_str(), so_sym_file.c_str()))
			perror("OnResponed get so file");

		print_thread_safe(g_log, "[OnResponed]取到so文件，存储路径：%s，链接路径：%s\n",
				so_store_file.c_str(), so_sym_file.c_str());
		chdir(key_vec[5].c_str());
		g_mgr.set_soini_flag(stg_id, key_vec[0]);
	}

	if (key_vec[0] == "ini") {		//ini文件
		auto& stg_id = key_vec[1];
		auto& ini_store_file = key_vec[2];

		FILE *fp = fopen(ini_store_file.c_str(), "w");
		fwrite(data->c_str(), 1, data->Length(), fp);
		fclose(fp);
		print_thread_safe(g_log, "[OnResponed]取到ini文件，存储路径：%s\n", ini_store_file.c_str());
		g_mgr.set_soini_flag(stg_id, key_vec[0]);
	}
}

void strategy_manager::set_soini_flag(const std::string& stg_id, const std::string& file_type) {
	if ("so" == file_type)
		m_id_key[stg_id].bit_flag |= 0x1;

	if ("ini" == file_type)
		m_id_key[stg_id].bit_flag |= 0x2;

	if (m_id_key[stg_id].bit_flag != 3)
		return;

	print_thread_safe(g_log, "已同时获取到so和ini，strategy_id=%s\n", stg_id.c_str());
	m_id_key.erase(stg_id);
	std::vector<std::string> args = {stg_id};
//	load_strategy(args);
}

int main(int argc, char *argv[]) {
	g_mgr.add_cmd("reqstg", "rs", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.request_strategy(args);
	}, "request strategy(so&ini) from remote server");

	g_mgr.add_cmd("load", "l", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.load_strategy(args);
	}, "load specified library");

	g_mgr.add_cmd("unload", "ul", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.unload_strategy(args);
	}, "unload specified library");

	g_mgr.add_cmd("showall", "sa", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.show_all();
	}, "show all strategies");

	g_mgr.add_cmd("showrun", "sr", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.show_run();
	}, "show running strategies");

	g_mgr.add_cmd("flush", "f", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.flush();
	}, "flush config");

	g_mgr.add_cmd("test", "t", [&](const std::vector<std::string>& args, crx::console *c)->void {
		g_mgr.test(args);
	}, "test if exist data");
	return g_mgr.run(argc, argv);
}
