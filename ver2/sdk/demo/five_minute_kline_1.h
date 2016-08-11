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
	TradeUUID uuid;

	ins_data()
	:m_status(STATUS_INIT)
	,m_seq(0)
	,m_five_greater_than_ten(false) {}
};

class five_minute_kline_1 : public strategy_base {
public:
	five_minute_kline_1();
	virtual ~five_minute_kline_1() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia);
	void execute_trade(const std::string& ins_id, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	std::string m_kline;
	int32_t m_12skline_index, m_5kavg_index, m_10kavg_index, m_60kavg_index;
	std::map<std::string, ins_data> m_ins_data;
};
