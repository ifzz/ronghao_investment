#include "five_minute_kline_1.h"

 std::shared_ptr<strategy_base> create_strategy() {
	 return std::make_shared<five_minute_kline_1>();
 }

five_minute_kline_1::five_minute_kline_1()
:m_status(STATUS_INIT)
,m_seq(0)
,m_five_greater_than_ten(false)
,m_12Skline_index(-1)
,m_5kavg_index(-1)
,m_10kavg_index(-1)
,m_60kavg_index(-1) {
}

void five_minute_kline_1::init() {
	strategy_base::init();
	m_12Skline_index = m_type_map["12秒kline"].type_index;
	m_5kavg_index = m_type_map["12秒kline"].tags["5均线"];
	m_10kavg_index = m_type_map["12秒kline"].tags["10均线"];
	m_60kavg_index = m_type_map["12秒kline"].tags["60均线"];
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
	if (group.diagrams.end() == group.diagrams.find(m_12Skline_index))
		return;

//	print_thread_safe("get new diagram package(%p) date=%d, seq=%d, status=%d, type=%d,"
//			"==>index=%d\n", item, item->data.diagram->base._date,
//			item->data.diagram->base._seq, item->data.diagram->base._state, item->data.diagram->base._type, item->index);

	MarketAnalyseDataBase *base = group.diagrams[m_12Skline_index].base;
	MarketAnalyseTagBase *five_avg = group.diagrams[m_12Skline_index].tags[m_5kavg_index];
	MarketAnalyseTagBase *ten_avg = group.diagrams[m_12Skline_index].tags[m_10kavg_index];
	MarketAnalyseTagBase *sixty_avg = group.diagrams[m_12Skline_index].tags[m_60kavg_index];

	if (five_avg && ten_avg && sixty_avg) {
		if (base->_seq <= m_seq)
			return;

		if (five_avg->_value == ten_avg->_value)		//当前这个包无效，丢包
			return;

		if (m_seq == 0) {
			if (five_avg->_value > ten_avg->_value)		// >
				m_five_greater_than_ten = true;
			else		// <
				m_five_greater_than_ten = false;
			m_seq = base->_seq;
			return;
		}

		m_seq = base->_seq;
		if (m_five_greater_than_ten) {		//5 > 10
			if (five_avg->_value < ten_avg->_value) {		//5 < 10
				m_five_greater_than_ten = false;		//发生翻转，5向下穿越10
			} else {
				return;		//状态未改变，忽略这个包
			}
		} else {		//5 < 10
			if (five_avg->_value > ten_avg->_value) {		//5 > 10
				m_five_greater_than_ten = true;		//发生翻转，5向上穿越10
			} else {
				return;		//状态未改变，直接忽略这个包
			}
		}

		//状态发生改变，做进一步操作，首先确定是否已经开仓，只有先开仓才能进行平仓
		MarketAnalyseKline *k_line = group.diagrams[m_12Skline_index].ext;
		datetime market;
		market.date = k_line->close_item.date;
		market.time = k_line->close_item.time;

		price_model model;
		model.type = ORDER_LIMIT;		//出限价单
		model.price = k_line->close_item.price;

		unsigned int seq = base->_seq;

		order_instruction oi;
		oi.type_index = m_12Skline_index;
		oi.seq = seq;
		oi.level = 1;
		oi.market = market;
		oi.model = model;
		if (STATUS_INIT == m_status) {		//此时还未开仓，那么先判断是不是要开仓
			if (sixty_avg->_value > five_avg->_value && sixty_avg->_value > ten_avg->_value) {
				//60均线在最上面，做空单
				if (false == m_five_greater_than_ten) {		//5向下穿越10，卖空
					m_status = STATUS_IN_OPEN_SELL;
					oi.flag = FLAG_OPEN;
					oi.direction = DIRECTION_SELL;
					request_trade(group.ins_id, oi);
				} else {
					return;		//还未开仓，无法平仓
				}
			} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
				if (true == m_five_greater_than_ten) {		//5向上穿越10，买多
					m_status = STATUS_IN_OPEN_BUY;
					oi.flag = FLAG_OPEN;
					oi.direction = DIRECTION_BUY;
					request_trade(group.ins_id, oi);
				} else {
					return;		//还未开仓，无法平仓
				}
			}
		} else {		//之前已经有过开仓平仓操作
			if (STATUS_IN_OPEN_BUY == m_status || STATUS_IN_OPEN_SELL == m_status) {
				//已经开仓，要平仓，此时只需要关注5和10均线是否发生反转
				if (STATUS_IN_OPEN_BUY == m_status) {		//买多
					m_status = STATUS_IN_CLOSE_SELL;
					oi.flag = FLAG_CLOSE;
					oi.direction = DIRECTION_SELL;
					request_trade(group.ins_id, oi);
				} else if (STATUS_IN_OPEN_SELL == m_status) {		//卖空
					m_status = STATUS_IN_CLOSE_BUY;
					oi.flag = FLAG_CLOSE;
					oi.direction = DIRECTION_BUY;
					request_trade(group.ins_id, oi);
				}
				return;
			}

			//还未开仓，要开仓，此时需要关注60均线的位置
			if (sixty_avg->_value > five_avg->_value && sixty_avg->_value > ten_avg->_value) {
				//60均线在最上面
				if (false == m_five_greater_than_ten) {		//5向下穿越10，卖空
					m_status = STATUS_IN_OPEN_SELL;
					oi.flag = FLAG_OPEN;
					oi.direction = DIRECTION_SELL;
					request_trade(group.ins_id, oi);
				}
			} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
				//60均线在最下面
				if (true == m_five_greater_than_ten) {		//5向上穿越10，买多
					m_status = STATUS_IN_OPEN_BUY;
					oi.flag = FLAG_OPEN;
					oi.direction = DIRECTION_BUY;
					request_trade(group.ins_id, oi);
				}
			}
		}
	}
}
