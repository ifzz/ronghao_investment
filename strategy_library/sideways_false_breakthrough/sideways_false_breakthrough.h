#pragma once

#include <deque>
#include "strategy_base.h"

typedef enum KLINE_NODE_STATUS {
	STATUS_INIT = 0,
	STATUS_BT_M_PLUS_FIVE,
	STATUS_BT_M_REDUCE_FIVE,
} KLINE_NODE_STATUS;

class sideways_false_breakthrough : public strategy_base {
public:
	sideways_false_breakthrough();
	virtual ~sideways_false_breakthrough();

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void construct_20_kline(depth_dia_group& group, int64_t tick);
	void reselect_20_kline(depth_dia_group& group, int64_t tick);
	void check_whether_satisfy_status(int64_t tick);
	void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	bool m_20_kline_is_ready;
	std::deque<depth_dia_group> m_group_queue;
	unsigned int m_max_highest_price, m_min_lowest_price, m_middle_price;

	float m_avg_volume;		//20个K线节点成交量的均值
	bool m_offset_open;			//指明是否已经开仓
	unsigned int m_open_highest, m_open_lowest, m_close_level;
	KLINE_NODE_STATUS m_node_sts;
	int32_t m_1minkline_index;
};
