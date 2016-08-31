#pragma once

#include <list>
#include <map>
#include <string.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "stock_data.h"

struct datetime {		//日期时间，需要将本地时间带给交易服务器
	unsigned int date;		//format: YYYYMMDD
	unsigned int time;		//format: HHMMSSmmm，最后三位为毫秒

	datetime()
	:date(0)
	,time(0) {}
};

datetime get_current_datetime();

enum ORDER_TYPE {
	ORDER_UNK = 0,
	ORDER_LIMIT,			//限价单
	ORDER_MARKET,		//市价单
	ORDER_LISTPRICE,	//对价单
};

enum TRADE_DIRECTION {
	DIRECTION_UNK = 0,
	DIRECTION_SELL,
	DIRECTION_BUY,
};

enum OFFSET_FLAG {
	FLAG_UNK = 0,
	FLAG_OPEN,
	FLAG_CLOSE,
};

struct price_model {
	ORDER_TYPE type;
	long long price;

	price_model()
	:type(ORDER_UNK)
	,price(0) {}
};

struct order_instruction {
	TradeUUID uuid;		//在开仓点由策略服务自动填充，在具体策略中保存此id，在平仓时关联之前的开仓点

	datetime market;			//行情日期时间
	TRADE_DIRECTION direction;		//买卖方向
	OFFSET_FLAG flag;						//开平标志
	long long price;		//价格

	unsigned int vol_cnt;				//手数
	char level;		//强弱信号

	int type_index;				//指标类型，前端画买卖点需要用到的数据
	int dia_seq;			//指标流水号
};

struct dia_data_tag {
	int type_index;
	std::map<std::string, int32_t> tags;		//当前这个data下的多个tag及其index之间的关联

	dia_data_tag()
	:type_index(-1) {}
};

//只在使用小端法的处理器平台（如x64）上有效
union wd_seq {
	struct {
		uint32_t seq;
		uint32_t date;
	};
	uint64_t vir_seq;
};

struct dia_group {
	MarketAnalyseDataBase *base;
	MarketAnalyseKline *ext;
	int mode;		//0删除 1新增 2修改
	std::vector<MarketAnalyseTagBase*> tags;		//一个data下面有多个tags
	std::vector<int> tag_mode;		//每个tag对应的操作模式

	dia_group()
	:base(nullptr)
	,ext(nullptr) {}
};

struct depth_dia_group {
	std::string ins_id;		//合约id
	std::shared_ptr<MarketDepthData> depth;	//深度行情
	std::map<int, std::list<dia_group>> dias;		//key: data_index, value: dia_group链表
};

class strategy_base {
public:
	strategy_base();
	virtual ~strategy_base();
	void *m_obj;

	virtual void execute(depth_dia_group& group) = 0;

	/**
	 * 如果需要额外的初始化步骤，或者在配置文件中需要加入额外的字段，那么可以重写以下read_config
	 * 和init这两个接口，执行顺序为read_config->init
	 */
	virtual void read_conf(std::map<std::string, const char*>& conf) {}
	virtual void init() {}

	/**
	 * 为某些策略加载历史图表数据，使这些策略能够快速初始化，例如需要的是5分钟K线，则以下参数初始化为：
	 * @id：合约id
	 * @param：5
	 * @name：分钟
	 * @class_name：kline
	 * @date：指定日期，从date开始加载一直到当前时点
	 * @f/args：回调及参数
	 */
	void for_each_his_dia(const std::string& id, long param, const std::string& name, const std::string& class_name, int date,
			std::function<void(dia_group&, void*)> f, void *args);

	/**
	 * 打印相关信息，用于调试
	 */
	void print_thread_safe(const char *format, ...);

	/**
	 * request_trade各参数的含义:
	 * ins_id：合约id
	 * oi：交易指令
	 */
	void request_trade(const std::string& ins_id, order_instruction& oi);

public:
	std::string m_strategy_name;				//策略名称
	std::string m_strategy_version;			//策略版本
	std::string m_strategy_ini;					//当前策略配置
	unsigned int m_strategy_time;			//策略开发时间，time format: YYYYMMDD
	std::string m_strategy_comment;		//策略备注
	std::string m_strategy_author;			//策略作者
	std::string m_strategy_developer;		//策略开发者

	std::map<std::string, dia_data_tag> m_type_map;
	std::map<std::string, ContractInfo> m_ins_info;
};

extern "C" std::shared_ptr<strategy_base> create_strategy();
