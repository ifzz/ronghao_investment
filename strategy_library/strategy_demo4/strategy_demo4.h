#pragma once

#include "strategy_base.h"

typedef enum {
	STATUS_INIT = 0,
	STATUS_N_READY,
	STATUS_N_PLUS_ONE_READY,
	STATUS_OPEN_BUY,
	STATUS_OPEN_SELL,
	STATUS_CLOSE_BUY,
	STATUS_CLOSE_SELL,
} STRATEGY_STATUS;

class strategy_demo4 : public strategy_base {
public:
	strategy_demo4();
	virtual ~strategy_demo4() {}

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void fi_init(MarketAnalyseDataBase *base, MarketAnalyseKline *ext);
	void fi_n_ready(MarketAnalyseDataBase *base, MarketAnalyseKline *ext);
	void fi_n_plus_one_ready(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group);
	void fi_open_buy(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group);
	void fi_open_sell(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group);

	 void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	STRATEGY_STATUS m_sts;
	unsigned int m_seq, m_init_k_num, m_n_plus_one_close_price;
	unsigned int m_n_plus_one_seq;
	unsigned int m_n_plus_two_highest_price, m_n_plus_two_lowest_price;
	int32_t m_1minkline_index;
};
