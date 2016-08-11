#include "history_mgr.h"

static E15_Log g_log;		//历史行情共用同一个日志
static std::mutex g_mtx_for_screen;
static std::mutex g_mtx_for_log;

void print_thread_safe(const char *format, ...) {
	va_list args;
	va_start(args, format);
	{
		std::unique_lock<std::mutex> lck(g_mtx_for_log);
		g_log.PrintfV(0, format, args);
	}
	va_end(args);
	if (true) {
		va_start(args, format);
		{
			std::unique_lock<std::mutex> lck(g_mtx_for_screen);
			vprintf(format, args);
		}
		va_end(args);
	}
}

data_parser::data_parser(const std::string& ins_id, unsigned int start, unsigned int end, history_mgr *ptr)
:crx::evd_thread_processor()
,m_ins_id(ins_id)
,m_current_date(start)
,m_start(start)
,m_end(end)
,m_mgr_ptr(ptr) {}

void data_parser::process_task(std::shared_ptr<crx::evd_thread_job> job) {
	static char time_buffer[64] = {0};
	timeval tv;
	gettimeofday(&tv, nullptr);
	tm *timeinfo = localtime(&tv.tv_sec);
	strftime(time_buffer, sizeof(time_buffer), "%H%M%S", timeinfo);
//	print_thread_safe("[time=%s.%ld]发送一次tick数据\n", time_buffer, tv.tv_usec/1000);

	//只需要转发tick级行情给实时行情服务器就可以了
	if (m_depth_list.empty()) {
		if (m_current_date > m_end) {
			m_mgr_ptr->notify_over(m_ins_id);
			return;
		}

		m_mgr_ptr->load_depth(m_ins_id.c_str(), this, m_current_date);
		if (m_depth_list.empty()) {
			print_thread_safe("不存在指定日期（%d）的tick级数据！\n", m_current_date);
			m_mgr_ptr->notify_over(m_ins_id);
			return;
		}
		print_thread_safe("当前合约（id=%s）指定日期%d的所有tick级行情加载完成！\n", m_ins_id.c_str(), m_current_date);
		m_current_date++;
	}

	//发送tick级行情
	E15_String *history_buf = new E15_String;
	auto& depth = m_depth_list.front();
	history_buf->Memcpy(m_ins_id.c_str(), m_ins_id.length());
	history_buf->Resize(16, 0);
	history_buf->Memcat((const char*)depth.get(), sizeof(MarketDepthData));
//	print_thread_safe("[time=%s.%ld]发送一次tick数据 depth=%p, len=%d, day=%d, time=%d\n", time_buffer, tv.tv_usec/1000,
//			history_buf->c_str(), history_buf->Length(), depth->base.nActionDay, depth->base.nTime);
	m_mgr_ptr->send_history(m_ins_id, history_buf, Stock_Msg_DepthMarket);
	m_depth_list.pop_front();
}

int data_parser::get_depth(void * obj,int market,const char * id,
		MarketDepthBase * base,MarketDepthExt * ext,MarketBidAsk * ba) {
//	printf("[%d:%d]\n", base->nActionDay, base->nTime);
	std::shared_ptr<MarketDepthData> depth = std::make_shared<MarketDepthData>();
	depth->base = *base;
	depth->ext = *ext;
	for (int i = 0; i < 9; ++i)
		depth->bid_ask[i] = ba[i];
	((data_parser*)obj)->m_depth_list.push_back(std::move(depth));
	return 1;
}

bool history_mgr::init(int argc, char *argv[]) {
	//全局日志初始化
	g_log.Init("history_md", 100);
	print_thread_safe("Complete the initialization of the global log object……\n");

	m_timer_th.start();
	m_thread_pool.start(10);
	m_trans->start();
	m_history_store = Create_E15_HistoryStore();
	m_history_store->Init();
	return true;
}

void history_mgr::destroy() {
	delete m_history_store;
	m_trans->stop();
	m_thread_pool.stop();
	m_timer_th.stop();
	print_thread_safe("history manager resource released!\n");
}

void history_mgr::timer_callback(int fd, void *args) {
	timer_args *ta = static_cast<timer_args*>(args);
	read(fd, &ta->cnt, sizeof(ta->cnt));
	std::shared_ptr<crx::evd_thread_job> job = std::make_shared<crx::evd_thread_job>(ta->type);
	ta->mgr_ptr->m_thread_pool.job_dispatch(job);
}

void history_mgr::load_depth(const char *ins_id, data_parser *parser, unsigned int date) {
	int market = MarketCodeById(ins_id);
	m_history_store->LoadDepthHistory(data_parser::get_depth, parser, market, ins_id, date);
}

void history_mgr::notify_over(const std::string& ins_id) {
	if (m_ins_info.end() == m_ins_info.find(ins_id))
		return;		//没有线程在处理这个合约，不用取消订阅

	m_timer_th.remove_epoll_event(m_ins_info[ins_id].fd);
	m_thread_pool.unregister_processor(m_ins_info[ins_id].ta->parser);
	m_ins_info.erase(ins_id);
	print_thread_safe("订阅合约ins_id=%s的指定历史行情发送完毕，任务已撤销\n", ins_id.c_str());
}

void history_mgr::history_subscribe(const char *id, unsigned int start, unsigned int end, unsigned int millisec) {
	if (m_ins_info.end() != m_ins_info.find(id)) {
		m_ins_info[id].sub_cnt++;
		return;
	}

	//合约还未订阅
	m_ins_info[id] = ins_info();
	m_ins_info[id].sub_cnt = 1;

	auto it = std::find(m_bitset.begin(), m_bitset.end(), 0);		//查找bitset中的空位
	int job_type = it-m_bitset.begin();
	if (it == m_bitset.end())
		m_bitset.push_back(1);
	else
		(*it) = 1;		//找到，直接在该位置1

	std::shared_ptr<timer_args> ta = std::make_shared<timer_args>(this, job_type);
	ta->parser = std::make_shared<data_parser>(id, start, end, this);
	ta->parser->register_type(job_type);
	m_thread_pool.register_processor(ta->parser);
	m_ins_info[id].ta = ta;
	print_thread_safe("\n订阅合约 id=%s，关注时间从 start=%d 到 end=%d，加入定时器事件，定时间隔为%dms\n",
			id, start, end, millisec);

	m_ins_info[id].fd = crx::create_timerfd(100, millisec);
	m_timer_th.add_epoll_event(m_ins_info[id].fd, timer_callback, ta.get());
}

void history_mgr::history_unsubscribe(const char *id) {
	if (m_ins_info.end() == m_ins_info.find(id))
		return;		//没有线程在处理这个合约，不用取消订阅

	m_ins_info[id].sub_cnt--;
	if (m_ins_info[id].sub_cnt)		//仍有需要订阅这个合约的策略机
		return;

	int job_type = m_ins_info[id].ta->type;
	m_bitset[job_type] = 0;

	m_timer_th.remove_epoll_event(m_ins_info[id].fd);
	close(m_ins_info[id].fd);
	m_thread_pool.unregister_processor(m_ins_info[id].ta->parser);
	m_ins_info.erase(id);
	print_thread_safe("退订合约 id=%s 成功！\n", id);
}

int main(int argc, char *argv[]) {
	history_mgr mgr;
	return mgr.run(argc, argv);
}
