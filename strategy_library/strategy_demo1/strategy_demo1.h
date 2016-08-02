#pragma once

#include "strategy_base.h"

enum STRATEGY_STATUS {
	STATUS_INIT = 0,
	STATUS_READY,
	STATUS_OPEN_SELL,
	STATUS_OPEN_BUY,
};

struct KLINE {
	unsigned int _seq;
	int64_t m_max_price;
	int64_t m_min_price;
};

struct ins_data {
	STRATEGY_STATUS m_sts;
	int64_t m_max_price, m_min_price;
	KLINE m_last_k_line;

	ins_data()
	:m_sts(STATUS_INIT)
	,m_max_price(0)
	,m_min_price(0) {}
};

class strategy_demo1 : public strategy_base {
public:
	strategy_demo1() : m_1minkline_index(-1) {
		 m_strategy_name = "strategy_demo1";
	 }
	virtual ~strategy_demo1() {}

public:
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	int32_t m_1minkline_index;
	std::map<std::string, ins_data> m_ins_data;
};
