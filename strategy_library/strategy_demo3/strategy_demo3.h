#pragma once

#include "strategy_base.h"

class strategy_demo3 : public strategy_base {
public:
	strategy_demo3() : m_12skline_index(-1) {
		m_strategy_name = "strategy_demo3";
	}
	virtual ~strategy_demo3() {}

public:
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	 void execute_trade(depth_dia_group& group, TRADE_DIRECTION direction, int price);

private:
	int64_t m_12skline_index;
	std::map<std::string, int64_t> m_ins_deal;
};
