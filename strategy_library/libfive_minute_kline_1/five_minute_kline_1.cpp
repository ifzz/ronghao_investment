#include "five_minute_kline_1.h"

 std::shared_ptr<strategy_base> create_strategy() {
	 return std::make_shared<five_minute_kline_1>();
 }

five_minute_kline_1::five_minute_kline_1()
:m_12skline_index(-1)
,m_5kavg_index(-1)
,m_10kavg_index(-1)
,m_60kavg_index(-1) {
	m_strategy_name = "five_minute_kline_1";
}

void five_minute_kline_1::init() {
	strategy_base::init();
	m_12skline_index = m_type_map["12秒kline"].type_index;
	m_5kavg_index = m_type_map["12秒kline"].tags["5均线"];
	m_10kavg_index = m_type_map["12秒kline"].tags["10均线"];
	m_60kavg_index = m_type_map["12秒kline"].tags["60均线"];
}

void five_minute_kline_1::execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
	MarketAnalyseKline *k_line = group.diagrams[m_12skline_index].ext;
 	datetime market;
 	market.date = k_line->close_item.date;
 	market.time = k_line->close_item.time;

 	price_model model;
 	model.type = ORDER_LIMIT;
 	model.price = k_line->close_item.price;

 	order_instruction oi;
 	size_t size = m_strategy_name.size() > 15 ? 15 : m_strategy_name.size();
 	memcpy(oi.stg_id, m_strategy_name.c_str(), size);
 	oi.stg_id[size] = '\0';
 	oi.type_index = m_12skline_index;
 	oi.seq = group.diagrams[m_12skline_index].base->_seq;
 	oi.level = 1;
 	oi.market = market;
 	oi.model = model;
 	oi.flag = flag;
 	oi.direction = direction;
 	request_trade(group.ins_id, oi);
}

/*
 * class_name ="kavg"
 * name = "均线"
 * param = "5/10/60"
 *
 * 5均线用红线表示
 * 10均线用蓝色表示
 *
 * 当60均线在最上时：
 * 		①5均线向下穿越10均线，卖空
 *
 * 		平仓只需要关注5和10均线
 * 		②向上穿越10均线，买平
 *
 * 	60均线在最下时：
 * 		①5均线向上穿越10均线，买多
 *
 * 		平仓操作只需要关注5和10均线是否反转
 * 		②向下穿越10均线，卖平
 *
 */
void five_minute_kline_1::execute(depth_dia_group& group) {
	if (group.diagrams.end() == group.diagrams.find(m_12skline_index))
		return;

	MarketAnalyseDataBase *base = group.diagrams[m_12skline_index].base;
	MarketAnalyseTagBase *five_avg = group.diagrams[m_12skline_index].tags[m_5kavg_index];
	MarketAnalyseTagBase *ten_avg = group.diagrams[m_12skline_index].tags[m_10kavg_index];
	MarketAnalyseTagBase *sixty_avg = group.diagrams[m_12skline_index].tags[m_60kavg_index];

	if (m_ins_data.end() == m_ins_data.find(group.ins_id))
		m_ins_data[group.ins_id] = ins_data();
	auto& data = m_ins_data[group.ins_id];

	if (five_avg && ten_avg && sixty_avg) {
		if (base->_seq <= data.m_seq)
			return;

		if (five_avg->_value == ten_avg->_value)		//当前这个包无效，丢包
			return;

		if (data.m_seq == 0) {
			if (five_avg->_value > ten_avg->_value)		// >
				data.m_five_greater_than_ten = true;
			else		// <
				data.m_five_greater_than_ten = false;
			data.m_seq = base->_seq;
			return;
		}

		data.m_seq = base->_seq;
		if (data.m_five_greater_than_ten) {		//5 > 10
			if (five_avg->_value < ten_avg->_value) {		//5 < 10
				data.m_five_greater_than_ten = false;		//发生翻转，5向下穿越10
			} else {
				return;		//状态未改变，忽略这个包
			}
		} else {		//5 < 10
			if (five_avg->_value > ten_avg->_value) {		//5 > 10
				data.m_five_greater_than_ten = true;		//发生翻转，5向上穿越10
			} else {
				return;		//状态未改变，直接忽略这个包
			}
		}

		//状态发生改变，做进一步操作，首先确定是否已经开仓，只有先开仓才能进行平仓

		if (STATUS_INIT == data.m_status) {		//此时还未开仓，那么先判断是不是要开仓
			if (sixty_avg->_value > five_avg->_value && sixty_avg->_value > ten_avg->_value) {
				//60均线在最上面，做空单
				if (false == data.m_five_greater_than_ten) {		//5向下穿越10，卖空
					data.m_status = STATUS_IN_OPEN_SELL;
					execute_trade(group, FLAG_OPEN, DIRECTION_SELL);
				} else {
					return;		//还未开仓，无法平仓
				}
			} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
				if (true == data.m_five_greater_than_ten) {		//5向上穿越10，买多
					data.m_status = STATUS_IN_OPEN_BUY;
					execute_trade(group, FLAG_OPEN, DIRECTION_BUY);
				} else {
					return;		//还未开仓，无法平仓
				}
			}
		} else {		//之前已经有过开仓平仓操作
			if (STATUS_IN_OPEN_BUY == data.m_status || STATUS_IN_OPEN_SELL == data.m_status) {
				//已经开仓，要平仓，此时只需要关注5和10均线是否发生反转
				if (STATUS_IN_OPEN_BUY == data.m_status) {		//买多
					data.m_status = STATUS_IN_CLOSE_SELL;
					execute_trade(group, FLAG_CLOSE, DIRECTION_SELL);
				} else if (STATUS_IN_OPEN_SELL == data.m_status) {		//卖空
					data.m_status = STATUS_IN_CLOSE_BUY;
					execute_trade(group, FLAG_CLOSE, DIRECTION_BUY);
				}
				return;
			}

			//还未开仓，要开仓，此时需要关注60均线的位置
			if (sixty_avg->_value > five_avg->_value && sixty_avg->_value > ten_avg->_value) {
				//60均线在最上面
				if (false == data.m_five_greater_than_ten) {		//5向下穿越10，卖空
					data.m_status = STATUS_IN_OPEN_SELL;
					execute_trade(group, FLAG_OPEN, DIRECTION_SELL);
				}
			} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
				//60均线在最下面
				if (true == data.m_five_greater_than_ten) {		//5向上穿越10，买多
					data.m_status = STATUS_IN_OPEN_BUY;
					execute_trade(group, FLAG_OPEN, DIRECTION_BUY);
				}
			}
		}
	}
}
