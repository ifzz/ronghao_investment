#pragma once

#include "strategy_base.h"

enum K_AVERAGE_STATUS {
	STATUS_INIT = 0,
	STATUS_IN_OPEN_BUY,
	STATUS_IN_CLOSE_BUY,
	STATUS_IN_OPEN_SELL,
	STATUS_IN_CLOSE_SELL,
} ;

struct ins_data {
	K_AVERAGE_STATUS m_status;
	unsigned int m_seq;
	bool m_five_greater_than_ten;

	ins_data()
	:m_status(STATUS_INIT)
	,m_seq(0)
	,m_five_greater_than_ten(false) {}
};

class five_minute_kline_1 : public strategy_base {
public:
	five_minute_kline_1();
	virtual ~five_minute_kline_1() {}

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	int32_t m_12skline_index, m_5kavg_index, m_10kavg_index, m_60kavg_index;
	std::map<std::string, ins_data> m_ins_data;
};
