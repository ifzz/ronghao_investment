#pragma once

#include "strategy_base.h"

class strategy_demo3 : public strategy_base {
public:
	strategy_demo3() : m_deal_price(-1), m_12skline_index(-1) {}
	virtual ~strategy_demo3() {}

public:
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	 void execute_trade(depth_dia_group& group, TRADE_DIRECTION direction, int price);

private:
	int64_t m_deal_price, m_12skline_index;
};
