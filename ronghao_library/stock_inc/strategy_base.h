#pragma once

#include <map>
#include <string.h>
#include <string>
#include <vector>
#include <memory>

#include "stock_data.h"
#include "stock_msg.h"

struct datetime {		//日期时间，需要将本地时间带给交易服务器
	unsigned int date;		//format: YYYYMMDD
	unsigned int time;		//format: HHMMSSmmm，最后三位为微妙

	datetime()
	:date(0)
	,time(0) {}
};

struct price_model {
	ORDER_TYPE type;
	long long price;

	price_model()
	:type(ORDER_UNK)
	,price(0) {}
};

struct order_instruction {
	datetime current;			//当前日期时间（**不用填）
	unsigned int strategy_id;			//策略id（**不用填）
	unsigned short src;		//下单源(策略服务器)id（**不用填）
	unsigned short trade_seq;		//交易流水号（**不用填）

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

struct dia_group {
	MarketAnalyseDataBase *base;
	MarketAnalyseKline *ext;
	std::vector<MarketAnalyseTagBase*> tags;

	dia_group()
	:base(nullptr)
	,ext(nullptr) {}
};

struct depth_dia_group {
	std::string ins_id;
	std::shared_ptr<MarketDepthData> depth;
	std::map<int, dia_group> diagrams;		//it->first: data_index

	depth_dia_group() {
		depth = std::make_shared<MarketDepthData>();
	}
};

class strategy_base {
public:
	strategy_base();
	virtual ~strategy_base();
	void *m_obj;

	/**
	 * execute中各个参数的含义:
	 * ins_id：合约id
	 * type：等于MARKET_DEPTH表示是深度行情，DIAGRAM_INFO表明是指标数据
	 * data_index：若当前数据为指标数据，那么data_index表示K线的类型
	 * data：具体的数据，若为MARKET_DEPTH则将该指针强转为MarketDepthData*类型，若为DIAGRAM_INFO
	 * 则将该指针强转为DiagramDataItem*类型
	 */
	virtual void execute(depth_dia_group& group) = 0;

	/**
	 * 如果需要额外的初始化步骤，或者在配置文件中需要加入额外的字段，那么可以重写以下read_config
	 * 和init这两个接口，在重写后的方法的入口点必须首先调用这两个接口，顺序为read_config->init
	 */
	virtual void read_config(const char *config);
	virtual void init();

	/**
	 * 打印相关信息，用于调试，这个函数是线程安全的
	 */
	void print_thread_safe(const char *format, ...);

	/**
	 * request_trade各参数的含义:
	 * instrument_id：合约id
	 * flag：开平标志
	 * direction：买卖方向
	 * level：信号强度
	 * market：在execute函数中当前处理的这个package_item的日期和时间
	 */
	void request_trade(const std::string& ins_id, order_instruction& oi);

protected:
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
