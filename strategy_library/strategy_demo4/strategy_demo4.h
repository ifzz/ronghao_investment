#pragma once

#include "strategy_base.h"

enum STRATEGY_STATUS {
	STATUS_INIT = 0,
	STATUS_N_READY,
	STATUS_N_PLUS_ONE_READY,
	STATUS_OPEN_BUY,
	STATUS_OPEN_SELL,
	STATUS_CLOSE_BUY,
	STATUS_CLOSE_SELL,
};

struct ins_data {
	STRATEGY_STATUS m_sts;
	unsigned int m_seq, m_init_k_num, m_n_plus_one_close_price;
	unsigned int m_n_plus_one_seq;
	unsigned int m_n_plus_two_highest_price, m_n_plus_two_lowest_price;
	TradeUUID uuid;

	ins_data()
	:m_sts(STATUS_INIT)
	,m_seq(-1)
	,m_init_k_num(0)
	,m_n_plus_one_close_price(0)
	,m_n_plus_one_seq(0)
	,m_n_plus_two_highest_price(0)
	,m_n_plus_two_lowest_price(0) {}
};

class strategy_demo4 : public strategy_base {
public:
	strategy_demo4() : m_1minkline_index(-1) {
		m_strategy_name = "strategy_demo4";
	}
	virtual ~strategy_demo4() {}

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void fi_init(dia_group& dia, ins_data& data);
	void fi_n_ready(dia_group& dia, ins_data& data);
	void fi_n_plus_one_ready(const std::string& ins_id, dia_group& dia, ins_data& data);
	void fi_open_buy(const std::string& ins_id, dia_group& dia, ins_data& data);
	void fi_open_sell(const std::string& ins_id, dia_group& dia, ins_data& data);

	void go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia);
	 void execute_trade(const std::string& ins_id, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	int32_t m_1minkline_index;
	std::map<std::string, ins_data> m_ins_data;
};
