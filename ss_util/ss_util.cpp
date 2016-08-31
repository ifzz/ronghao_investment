#include "rhafx.h"

E15_Log g_log;		//整个策略框架共用同一个日志
crx::evd_thread_pool g_log_th;
std::shared_ptr<log_handle> g_log_hd;

stg_config g_conf;

datetime get_current_datetime() {
	datetime dt;
	char time_buffer[64] = {0};

	timeval tv;
	gettimeofday(&tv, nullptr);
	tm *timeinfo = localtime(&tv.tv_sec);
	strftime(time_buffer, sizeof(time_buffer), "%Y%m%d", timeinfo);
	dt.date = atoi(time_buffer);
	strftime(time_buffer, sizeof(time_buffer), "%H%M%S", timeinfo);
	dt.time = atoi(time_buffer)*1000+tv.tv_usec/1000;		//时间精确到毫秒级
	return dt;
}

void log_handle::process_task(std::shared_ptr<crx::evd_thread_job> job) {
	auto j = std::dynamic_pointer_cast<log_job>(job);
	j->log->Printf(0, j->print_str.c_str());
	if (j->print_screen)
		printf("%s", j->print_str.c_str());
}

void print_thread_safe(E15_Log& log, const char *format, ...) {
	char log_buf[4096] = {0};
	va_list args;
	va_start(args, format);
	vsprintf(log_buf, format, args);
	va_end(args);
	auto job = std::make_shared<log_job>(log_buf, &log, true);
	g_log_th.job_dispatch(job);
}

void strategy_base::print_thread_safe(const char *format, ...) {
	sb_impl *impl = static_cast<sb_impl*>(m_obj);
	va_list args;
	va_start(args, format);
	vsprintf(impl->format_buf, format, args);
	va_end(args);
	auto job = std::make_shared<log_job>(impl->format_buf, &impl->m_log, impl->m_print_screen);
	g_log_th.job_dispatch(job);
}

strategy_base::strategy_base()
:m_strategy_time(0) {
	m_obj = new sb_impl(this);
}

strategy_base::~strategy_base() {
	sb_impl *impl = static_cast<sb_impl*>(m_obj);
	if (impl->m_trade_import) {
		fclose(impl->m_trade_import);
		impl->m_trade_import = nullptr;
	}
	delete impl;
}

void sb_impl::read_config(const char *config) {
	E15_Ini ini;
	ini.Read(config);
	m_this_stg->m_strategy_ini = config;

	ini.SetSection("share");
	int sub_all = 1;
	ini.Read("sub_all", sub_all);

	if (sub_all) {
		m_mgr_ptr->for_each_ins([&](const std::string& ins, const ContractInfo& info, void *args)->void {
			m_this_stg->m_ins_info[ins] = info;
		}, nullptr);
	} else {
		std::string ins_id = ini.ReadString("ins_id", "");		//获取关注的合约id
		for (auto& id : crx::split(ins_id, ";"))
			m_this_stg->m_ins_info[id] = m_mgr_ptr->get_ins_info(id);
	}

	ini.SetSection("custom");
	std::map<std::string, const char*> cus_ini;
	const char *key = ini.GetFirstKey();
	do {
		cus_ini[key] = ini.ReadString(key, "");
		key = ini.GetNextKey();
	} while (key);

	auto& dia_type = cus_ini["diagram_type"];		//关注的指标类型
	for (auto& dt : crx::split(dia_type, ";")) {
		auto desc = crx::split(dt, ":");
		for (unsigned int i = 0; i < g_data_tag_info.m_list->Count(); ++i) {
			DataDescription *ddesc = (DataDescription*)g_data_tag_info.m_list->At(i, 0);
			if (!strcmp(ddesc->m_dt.class_name, desc[0].c_str()) &&
					!strcmp(ddesc->m_dt.name, desc[1].c_str()) &&
					ddesc->m_dt.param == atoi(desc[2].c_str())) {
				std::string kline_name = desc[2]+desc[1]+desc[0];

				dia_data_tag dia_map;
				dia_map.type_index = ddesc->m_dt.type_index;
				for (unsigned int j = 0; j < ddesc->m_sub->Count(); ++j) {
					DataDescription *tag_desc = (DataDescription*)ddesc->m_sub->At(j, 0);
					std::string kavg_name = std::to_string(tag_desc->m_dt.param)+tag_desc->m_dt.name+tag_desc->m_dt.class_name;
					dia_map.tags[kavg_name] = tag_desc->m_dt.type_index;
				}
				m_this_stg->m_type_map[kline_name] = dia_map;
				break;
			}
		}
	}
	if (cus_ini.end() != cus_ini.find("print_screen")) {
		m_print_screen = atoi(cus_ini["print_screen"]);
		cus_ini.erase("print_screen");
	}

	cus_ini.erase("diagram_type");
	m_this_stg->read_conf(cus_ini);
}

void sb_impl::on_init() {
	//策略私有日志初始化
	m_log.Init(m_this_stg->m_strategy_name.c_str(), 100);
	m_stg_id = m_mgr_ptr->get_usable_stg_id();

	//记录该策略对应的全局配置信息
	char strategy_config_buffer[1024] = {0};
	std::string strategy_config;
	sprintf(strategy_config_buffer, "\n###############################################################\n"
			"当前正在运行的策略的全局信息如下："
			"\n\t==>策略名称：%s，"
			"\n\t==>策略版本：%s，"
			"\n\t==>策略作者：%s，"
			"\n\t==>策略开发者：%s，"
			"\n\t==>创建策略库的时间：%d，"
			"\n\t==>策略配置名：%s，"
			"\n\t==>策略备注：%s",
				m_this_stg->m_strategy_name.c_str(),
				m_this_stg->m_strategy_version.c_str(),
				m_this_stg->m_strategy_author.c_str(),
				m_this_stg->m_strategy_developer.c_str(),
				m_this_stg->m_strategy_time,
				m_this_stg->m_strategy_ini.c_str(),
				m_this_stg->m_strategy_comment.c_str());
	strategy_config += strategy_config_buffer;

	auto type = m_this_stg->m_type_map.begin();
	if (type != m_this_stg->m_type_map.end()) {
		sprintf(strategy_config_buffer, "\n\t==>当前这个策略主要关注的指标数据包括：%s（%d）",
				type->first.c_str(), type->second.type_index);
		strategy_config += strategy_config_buffer;

		while (++type != m_this_stg->m_type_map.end()) {		//记录关注的所有指标数据类型
			sprintf(strategy_config_buffer, ", %s（%d）", type->first.c_str(), type->second.type_index);
			strategy_config += strategy_config_buffer;
		}
	}
	strategy_config += "\n###############################################################\n";
	m_this_stg->print_thread_safe(strategy_config.c_str());

	file_dia.base = new MarketAnalyseDataBase;
	file_dia.ext = new MarketAnalyseKline;
	for (int i = 0; i < 32; ++i) {
		file_dia.tags.push_back(new MarketAnalyseTagBase);
		file_dia.tag_mode.push_back(-1);
	}
	m_this_stg->init();
	delete file_dia.base;
	delete file_dia.ext;
	for (auto& tag : file_dia.tags)
		delete tag;
}

void strategy_base::request_trade(const std::string& ins_id, order_instruction& oi) {
	sb_impl *impl = static_cast<sb_impl*>(m_obj);
	if (FLAG_OPEN == oi.flag) {
		datetime dt = get_current_datetime();
		oi.uuid.date = dt.date;
		oi.uuid.time = dt.time;
		oi.uuid.strategy_id = impl->m_stg_id;
		oi.uuid.src = 0;
		oi.uuid.seq = impl->m_seq_id++;
	}
	impl->m_mgr_ptr->send_instruction(ins_id, oi);

	/*
	 * * 	接下来要做统计：
	 * * 	1、在什么时间点以什么价格进行买卖，在该点是平仓还是空仓
	 *				①时间点包括行情服务器给的行情中的时间点以及当前时间点
	 *				②价格指的是一个价格模型，这个模型有两个字段，第一个字段指的是当前这个单是以限价、市价还是对价形式
	 *				下单，第二个字段就是具体的价格，有些模型不需要给字段的话直接设置为0就可以了
	 * * 	2、策略名称（一定要对应到参数改变，给这组参数配置一个id，打印时直接打印id，根据id马上就可以定位到
	 * * 	具体的参数配置，实际上这个策略的配置在初始化阶段就已经固定了，所以在策略的整个运行周期中只需要打印一次
	 * 		就可以了，可以选择在策略加载时进行打印）、合约ID、哪一类type index的diagram data
	 *
	 */

	std::string offset_flag;
	if (FLAG_OPEN == oi.flag)
		offset_flag = "开仓";
	else if (FLAG_CLOSE == oi.flag)
		offset_flag = "平仓";
	else
		offset_flag = "开平标志未知";

	std::string trade_direction;
	if (DIRECTION_BUY == oi.direction)
		trade_direction = "买";
	else if (DIRECTION_SELL == oi.direction)
		trade_direction = "卖";
	else
		trade_direction = "买卖方向未知";

	std::string diagram = "深度行情";
	for (unsigned int i = 0; i < g_data_tag_info.m_list->Count(); ++i) {
		DataDescription *data_desc = (DataDescription*)g_data_tag_info.m_list->At(i, 0);
		if ((unsigned int)oi.type_index == data_desc->m_dt.type_index)
			diagram = std::to_string(data_desc->m_dt.param)+data_desc->m_dt.name+data_desc->m_dt.class_name;
	}

	char buf[256] = {0};
	//本地日期#时间 策略id 下单源id 交易流水号 合约id 指标类型#指标流水号 开平#交易方向 价格#手数#信号 行情日期#时间
	sprintf(buf, "本地%d#%d %d %d %d  %s  %s#%d  %s#%s  %lld#%d#%d  行情%d#%d\n",
			oi.uuid.date, oi.uuid.time, oi.uuid.strategy_id, oi.uuid.src, oi.uuid.seq, ins_id.c_str(), diagram.c_str(), oi.dia_seq,
			offset_flag.c_str(), trade_direction.c_str(), oi.price, oi.vol_cnt, oi.level, oi.market.date, oi.market.time);
	print_thread_safe("%s", buf);

	if (impl->m_current_date != oi.uuid.date) {		//每天新建一个文件，将当日所有下单记录保存在同一个文件中
		if (impl->m_trade_import)		//首先将上一日的文件关闭
			fclose(impl->m_trade_import);

		impl->m_current_date = oi.uuid.date;
		char file[256] = {0};
		if (g_conf.for_produce)		//生产环境可以配置买卖点日志的根目录
			sprintf(file, "%s/%s-%d.log", g_conf.db_import.c_str(), m_strategy_name.c_str(), impl->m_current_date);
		else		//测试环境直接写死就可以了
			sprintf(file, "database_import/%s-%d.log", m_strategy_name.c_str(), impl->m_current_date);
		impl->m_trade_import = fopen(file, "w");		//再新开一个文件存储
		print_thread_safe("日期更新，新建一个当前策略的买卖点日志current date = %d, file name = %s, file ptr=%p\n",
				impl->m_current_date, file, impl->m_trade_import);
	}
	fprintf(impl->m_trade_import, "%s", buf);
	fflush(impl->m_trade_import);
}

void strategy_base::for_each_his_dia(const std::string& id, long param, const std::string& name, const std::string& class_name,
		int date, std::function<void(dia_group&, void*)> f, void *args) {
	if (date > 0)		//0表示当天历史，-1表示前一天历史，依次类推
		return;

	sb_impl *impl = static_cast<sb_impl*>(m_obj);
	auto& cache = impl->cache[id];
	cache.f = std::move(f);
	cache.args = args;
	impl->file_dia.mode = -1;

	short store_level = 0;
	if (name == "日" || name == "周" || name == "月")
		store_level = 1;

	datetime cdt = get_current_datetime();
	date += cdt.date;

	//历史文件
	impl->file_dia.base->_date = date;
	impl->file_dia.base->_seq = 0;
	MarketDataType dt = {0, 0, param, name.c_str(), class_name.c_str(), store_level};
	impl->m_mgr_ptr->load_dia_history(id, dt, date, cdt.date, m_obj);

	int market = MarketCodeById(id.c_str());
	DiagramDataMgr *stock = DiagramDataMgr_PeekData(market, id.c_str());
	if (!stock)
		return;

	std::string kline = std::to_string(param)+name+class_name;
	int data_index = m_type_map[kline].type_index;
	DiagramDataHandler *h = (DiagramDataHandler*)stock->factory->m_data->At(data_index, 0);
	if (!h)
		return;

	cache.cache_dia[data_index].mode = -1;
	for (int i = 0; i < 32; ++i) {
		cache.cache_dia[data_index].tags.push_back(nullptr);
		cache.cache_dia[data_index].tag_mode.push_back(-1);
	}

	//在加载历史文件时有最后一根K线未被执行，需要和历史缓存比较是否是已完成
	stock->lock.Lock();
	impl->m_mgr_ptr->load_dia_hiscache(id, data_index, h, store_level, m_obj);		//历史缓存
	stock->lock.Unlock();
}

ss_util::ss_util()
:m_ins_list(nullptr)
,m_diagram_info(nullptr)
,m_history_store(nullptr) {}

ss_util::~ss_util() {
	if (m_ins_list)
		delete m_ins_list;
	if (m_diagram_info)
		delete m_diagram_info;
}

void ss_util::parse_ini() {
	E15_Ini ini;
	ini.Read("ini/config.ini");
	ini.SetSection("client");
	g_conf.ip = ini.ReadString("addr", "");
	ini.Read("port", g_conf.port);
	g_conf.user = ini.ReadString("user", "test");
	g_conf.passwd = ini.ReadString("password", "123456");

	ini.SetSection("setting");
	ini.Read("sub_all", g_conf.sub_all);
	ini.Read("conn_real", g_conf.conn_real);
	ini.Read("for_produce", g_conf.for_produce);
	ini.Read("threads_num", g_conf.threads_num);
	g_conf.stg_dir = ini.ReadString("stg_dir", "");
	g_conf.db_import = ini.ReadString("db_import", "");
	if (access(g_conf.db_import.c_str(), F_OK))		//创建数据导入目录
		mkdir(g_conf.db_import.c_str(), 0755);

	g_conf.md_role = ini.ReadString("md_role", "");
	g_conf.his_md_role = ini.ReadString("his_md_role", "");
	g_conf.trade_role = ini.ReadString("trade_role", "");
	g_conf.cli_prx_role = ini.ReadString("cli_prx_role", "");

	g_conf.so_addr = ini.ReadString("so_addr", "");
	ini.Read("so_port", g_conf.so_port);
}

void ss_util::global_log_init() {
	//全局日志初始化
	g_log_hd = std::make_shared<log_handle>();
	g_log_hd->register_type(1);

	g_log_th.start(1);
	g_log_th.register_processor(g_log_hd);

	m_history_store = Create_E15_HistoryStore();
	m_history_store->Init();
}

void ss_util::global_log_destroy() {
	delete m_history_store;

	//终止日志线程
	g_log_th.unregister_processor(g_log_hd);
	g_log_th.stop();
}

ContractInfo ss_util::get_ins_info(const std::string& id) {
	return m_ins_info[id].detail;
}

void ss_util::for_each_ins(std::function<void(const std::string&, const ContractInfo&, void*)> f, void *args) {
	for (auto& pair : m_ins_info)
		f(pair.first, pair.second.detail, args);
}

static int64_t s_ins_type = 0;
int ss_util::handle_contract_info(E15_Key *key, E15_Value *info, ss_util *mgr_ptr) {
	const char * id = key->GetS();
	if( !id )
		return 0;

	const char * name = info->BytesS("name",0);
	if( !name )
		return 0;

	int market = info->BaseS("market"); //MarketCodeById(id);
	DiagramDataMgr * data = DiagramDataMgr_GetData(market,id,name,1);
	data->info.price_tick = info->BaseS("tick");
	data->info.Multiple = info->BaseS("Multiple");

	auto& ins_info = mgr_ptr->m_ins_info;
	ins_info[id].type = s_ins_type++;
	ins_info[id].detail = data->info;
//	printf("contract info:: id=%s, tick=%ld, multiple=%ld, type=%ld\n", id, data->info.price_tick,
//			data->info.Multiple, ins_info[id].type);
	return 0;
}

void ss_util::parse_instrument_list(const char *data, int len) {
	E15_Zip unzip;
	E15_String unzip_buffer;
	unzip.unzip_start(&unzip_buffer);
	unzip.unzip(data, len);
	unzip.unzip_end();

	E15_ValueTable instrument_info;
	instrument_info.Import(unzip_buffer.c_str(), unzip_buffer.Length());
	instrument_info.each((int (*)(E15_Key * key, E15_Value * info, void *))handle_contract_info, this);
}

void ss_util::parse_diagram_info(const char *data, int len) {
	if(g_data_tag_info.m_list->Count() == 0)
		g_data_tag_info.InitDescription(data, len);
	g_market_data->Each([](StockData *data, void *param)->void {
		if (data->data) {
//			E15_Debug::Printf(0, "%s Init cache Factory \n", data->code.c_str());
			DiagramDataMgr *mgr = (DiagramDataMgr*)data->data;
			mgr->factory->Init(g_data_tag_info.m_list);
		}
	}, 0, E15_StockMarketCode_all);
}

int ss_util::handle_diagram_item(E15_Key * key,E15_Value * info,DiagramDataMgr *stock)
{
	unsigned int parent_index = info->BaseS("pi");
	unsigned int index = info->BaseS("di");
	int mode = info->BaseS("mode");
	E15_String * s = info->ValueS("data")->GetString();

	if( parent_index == (unsigned int)-1) //这个是数据
	{
		stock->factory->OnData(stock->depth, mode, index, s);
		return 0;
	}

	stock->factory->OnTag(stock->depth, mode,parent_index,index,s);
	return 0;
}

void ss_util::load_dia_history(const std::string& id, MarketDataType& dt, unsigned int date, unsigned int cdate, void *obj) {
	int market = MarketCodeById(id.c_str());
	if (!dt.store_level) {		//存储级别为0，加载的是day目录下的历史图表
		for (; date <= cdate; ++date)		//包括当天的历史
			m_history_store->LoadDiaHistory(ss_util::get_dia_history, obj, market, id.c_str(), &dt, date);
		return;
	}

	//存储级别为1，加载的是year目录下的历史图表
	for (; date < cdate; ++date)		//不包括当天的历史
		m_history_store->LoadDiaHistory(ss_util::get_dia_history, obj, market, id.c_str(), &dt, date, 0, 1, 1);
}

int ss_util::get_dia_history(void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,
			HistoryDiaBase * base,const char * ext_data,int len) {
	sb_impl *impl = static_cast<sb_impl*>(obj);
	if (!tt) {		//当前得到的是一个data
		if (1 == impl->file_dia.mode) {
			auto& cache = impl->cache[id];
			cache.f(impl->file_dia, cache.args);
		}

		*impl->file_dia.base = *base->data_base;
		if (!impl->file_dia.base->_state)
			impl->file_dia.base->_state = 2;
		if (len && len == sizeof(MarketAnalyseKline))
			memcpy(impl->file_dia.ext, ext_data, len);
		impl->file_dia.mode = 1;
		for (auto& tag_mode : impl->file_dia.tag_mode)
			tag_mode = -1;
	} else {		//得到的是一个tag
		std::string data_name = std::to_string(dt->param)+dt->name+dt->class_name;
		std::string tag_name = std::to_string(tt->param)+tt->name+tt->class_name;
		int tag_idx = impl->m_this_stg->m_type_map[data_name].tags[tag_name];
		*impl->file_dia.tags[tag_idx] = *base->tag_base;
		impl->file_dia.tag_mode[tag_idx] = 1;
	}
	return 1;
}

void ss_util::load_dia_hiscache(const std::string& id, int data_index, DiagramDataHandler *h, short store_level, void *obj) {
	sb_impl *impl = static_cast<sb_impl*>(obj);
	if (!h->m_data->Count())
		return;		//没有缓存，直接退出

	DiagramDataItem *item = (DiagramDataItem*)h->m_data->Tail(0);
	if (-1 == impl->file_dia.mode) {		//没有加载到历史文件，一直加载直到缓存的最后第二根K线
		wd_seq fseq, cseq;
		fseq.date = impl->file_dia.base->_date;
		fseq.seq = impl->file_dia.base->_seq;

		for (unsigned long i = 0; i < h->m_data->Count()-1; ++i) {
			DiagramDataItem* it = (DiagramDataItem*)h->m_data->At(i, 0);
			cseq.date = it->base._date;
			cseq.seq = it->base._seq;
			if (cseq.vir_seq < fseq.vir_seq)
				continue;

			load_cache_forstg(id, data_index, it, impl, true);
		}
		load_cache_forstg(id, data_index, item, impl, false);
		return;
	}

	if (1 == impl->file_dia.mode) {
		while (true) {
			if (item->base._date == impl->file_dia.base->_date &&
					item->base._seq == impl->file_dia.base->_seq)		//在缓存中找到历史文件中的最后一根K线
				break;

			DiagramDataItem *temp = (DiagramDataItem*)item->Pre();
			if (!temp) {		//已比较到缓存中的第一根K线，说明没找到历史文件的最后一根K线
				auto& cache = impl->cache[id];
				cache.f(impl->file_dia, cache.args);
				break;
			}
			item = temp;
		}

		//从当前这个item一直执行到缓存的最后第二根K线
		while (item->Next()) {
			load_cache_forstg(id, data_index, item, impl, true);
			item = (DiagramDataItem*)item->Next();
		}
		load_cache_forstg(id, data_index, item, impl, false);
	}
}

void ss_util::load_cache_forstg(const std::string& id, int data_index, DiagramDataItem *item, sb_impl *impl, bool exec) {
	auto& cache = impl->cache[id];
	cache.cache_dia[data_index].base = &item->base;
	if (item->pri && item->pri->Length() == sizeof(MarketAnalyseKline))
		cache.cache_dia[data_index].ext = (MarketAnalyseKline*)item->pri->c_str();
	cache.cache_dia[data_index].mode = 1;
	for (auto& tag_mode : cache.cache_dia[data_index].tag_mode)
		tag_mode = -1;

	for (int i = 0; i < item->tag_cnt; ++i) {
		DiagramTag *tag = item->PeekTag(i);
		if (!tag)
			continue;

		cache.cache_dia[data_index].tags[i] = &tag->base;
		cache.cache_dia[data_index].tag_mode[i] = 1;
	}
	if (exec)
		cache.f(cache.cache_dia[data_index], cache.args);
}

void ss_util::parse_cache_diagroup(E15_ServerCmd *cmd, const char *data, int len) {
	int market = MarketCodeById(data);
	DiagramDataMgr *stock = DiagramDataMgr_GetData(market, data, 0, 1);
	m_unzip_buffer.Reset();
	m_unzip.unzip_start(&m_unzip_buffer);
	m_unzip.unzip(data+DEPTH_MARKET_HEAD_LEN, len-DEPTH_MARKET_HEAD_LEN);
	m_unzip.unzip_end();

	if (Stock_Msg_DiagramCacheData == cmd->cmd)
		stock->factory->LoadCacheData(m_unzip_buffer.c_str(), m_unzip_buffer.Length(), cmd->receiver.l, cmd->status);
	else		//Stock_Msg_DiagramCacheTag == cmd->cmd
		stock->factory->LoadCacheTag(m_unzip_buffer.c_str(), m_unzip_buffer.Length(), cmd->receiver.l, cmd->receiver.h, cmd->status);
}

void ss_util::make_index_for_cache() {
	dia_group group;
	for (auto& info : m_ins_info) {
		int market = MarketCodeById(info.first.c_str());
		DiagramDataMgr *stock = DiagramDataMgr_GetData(market, info.first.c_str(), 0, 1);
		for (unsigned long i = 0; i < stock->factory->m_data->Count(); ++i) {
			auto& cache_list = info.second.dia_cache[i];
			DiagramDataHandler *h = (DiagramDataHandler*)stock->factory->m_data->At(i, 0);
			for (size_t j = 0; j < h->m_data->Count(); ++j) {
				DiagramDataItem *item = (DiagramDataItem*)h->m_data->At(j, 0);
				//base & ext
				group.base = &item->base;
				if (!group.base->_state)
					group.base->_state = 2;
				group.ext = (MarketAnalyseKline*)item->pri->c_str();
				group.mode = 1;

				//tag
				for (int k = 0; k < item->tag_cnt; ++k) {
					DiagramTag *tag = item->PeekTag(k);
					if (tag) {
						group.tags.push_back(&tag->base);
						group.tag_mode.push_back(1);
					} else {
						group.tags.push_back(nullptr);
						group.tag_mode.push_back(-1);
					}
				}

				cache_list.push_back(group);
				group.tags.clear();
				group.tag_mode.clear();
			}
		}
	}
}

depth_dia_group ss_util::parse_diagram_group(const char *data, int len) {
	int market = MarketCodeById(data);
	DiagramDataMgr *stock = DiagramDataMgr_GetData(market, data, 0, 1);
	m_unzip_buffer.Reset();
	m_unzip.unzip_start(&m_unzip_buffer);
	m_unzip.unzip(data+DEPTH_MARKET_HEAD_LEN, len-DEPTH_MARKET_HEAD_LEN);
	m_unzip.unzip_end();

	m_vt.Import(m_unzip_buffer.c_str(), m_unzip_buffer.Length());
	unsigned long depth_len = 0;
	stock->depth = (MarketDepthData*)m_vt.BytesS("depth", &depth_len);
	assert(stock->depth && depth_len == sizeof(MarketDepthData));		//一定会有深度行情

	depth_dia_group group;
	group.ins_id = data;
	group.depth = std::make_shared<MarketDepthData>();
	*group.depth = *stock->depth;

	E15_ValueTable *dia_vt = m_vt.TableS("dia");
	if (dia_vt) {
		stock->lock.Lock();
		dia_vt->each((int (*)(E15_Key * key,E15_Value * info,void *))handle_diagram_item, stock);
		stock->lock.Unlock();
		for (auto& raw : g_dia_deq) {
			dia_group g;
			g.base = &raw.data->base;
			g.ext = (MarketAnalyseKline*)raw.data->pri->c_str();
			g.mode = raw.mode;

			for (int i = 0; i < raw.data->tag_cnt; ++i) {
				if (raw.data->tags[i]) {		//tag存在，删除操作也要通知策略
					g.tags.push_back(&raw.data->tags[i]->base);
					if (raw.tag_mode.end() != raw.tag_mode.find(i))
						g.tag_mode.push_back(raw.tag_mode[i]);
					else
						g.tag_mode.push_back(-1);
				} else {
					g.tags.push_back(nullptr);
					g.tag_mode.push_back(-1);
				}
			}
			group.dias[raw.data_index].push_back(g);
		}
		for (auto& dia : group.dias) {
			if (dia.second.size() == 1)
				continue;
			dia.second.sort([](const dia_group& i, const dia_group& j)->bool {
				wd_seq iseq, jseq;
				iseq.date = i.base->_date;
				iseq.seq = i.base->_seq;
				jseq.date = j.base->_date;
				jseq.seq = j.base->_seq;
				return iseq.vir_seq < jseq.vir_seq;
			});
		}
		g_dia_deq.clear();
	}
	m_vt.Reset();
	return group;
}
