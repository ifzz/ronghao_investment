#pragma once

#include "strategy_base.h"

typedef enum {
	STATUS_INIT = 0,
	STATUS_READY,
	STATUS_OPEN_SELL,
	STATUS_OPEN_BUY,
} STRATEGY_STATUS;

typedef struct KLINE {
	unsigned int _seq;
	int64_t m_max_price;
	int64_t m_min_price;
} KLINE;

class strategy_demo1 : public strategy_base {
public:
	strategy_demo1();
	virtual ~strategy_demo1() {}

public:
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	STRATEGY_STATUS m_sts;
	int64_t m_max_price, m_min_price;
	KLINE m_last_k_line;
	int32_t m_1minkline_index;
};
