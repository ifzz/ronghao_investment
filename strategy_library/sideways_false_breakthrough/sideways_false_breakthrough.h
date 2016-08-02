#pragma once

#include <deque>
#include "strategy_base.h"

typedef enum KLINE_NODE_STATUS {
	STATUS_INIT = 0,
	STATUS_BT_M_PLUS_FIVE,
	STATUS_BT_M_REDUCE_FIVE,
} KLINE_NODE_STATUS;

struct ins_data {
	bool m_20_kline_is_ready;
	std::deque<depth_dia_group> m_group_queue;
	unsigned int m_max_highest_price, m_min_lowest_price, m_middle_price;

	float m_avg_volume;		//20个K线节点成交量的均值
	bool m_offset_open;			//指明是否已经开仓
	unsigned int m_open_highest, m_open_lowest, m_close_level;
	KLINE_NODE_STATUS m_node_sts;

	ins_data()
	:m_20_kline_is_ready(false)
	,m_max_highest_price(0)
	,m_min_lowest_price((unsigned int)UINTMAX_MAX)
	,m_middle_price(0)
	,m_avg_volume(0)
	,m_offset_open(false)
	,m_open_highest(0)
	,m_open_lowest(0)
	,m_close_level(0)
	,m_node_sts(STATUS_INIT) {}
};

class sideways_false_breakthrough : public strategy_base {
public:
	sideways_false_breakthrough() : m_1minkline_index(-1) {
		 m_strategy_name = "sideways_false_breakthrough";
	 }
	virtual ~sideways_false_breakthrough() {}

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void construct_20_kline(depth_dia_group& group, int64_t tick, ins_data& data);
	void reselect_20_kline(depth_dia_group& group, int64_t tick, ins_data& data);
	void check_whether_satisfy_status(int64_t tick, ins_data& data);
	void execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction);

private:
	int32_t m_1minkline_index;
	std::map<std::string, ins_data> m_ins_data;
};
