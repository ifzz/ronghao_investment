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

void five_minute_kline_1::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
}

void five_minute_kline_1::init() {
	m_12skline_index = m_type_map[m_kline].type_index;
	m_5kavg_index = m_type_map[m_kline].tags["5均线kavg"];
	m_10kavg_index = m_type_map[m_kline].tags["10均线kavg"];
	m_60kavg_index = m_type_map[m_kline].tags["60均线kavg"];
}

void five_minute_kline_1::execute_trade(const std::string& ins_id, dia_group& dia,
		OFFSET_FLAG flag, TRADE_DIRECTION direction) {
 	datetime market;
 	market.date = dia.ext->close_item.date;
 	market.time = dia.ext->close_item.time;

 	order_instruction oi;
 	if (FLAG_CLOSE == flag)
 		oi.uuid = m_ins_data[ins_id].uuid;

 	oi.market = market;
 	oi.direction = direction;
 	oi.flag = flag;
 	oi.price = dia.ext->close_item.price;

 	oi.vol_cnt = 1;
 	oi.level = 1;

 	oi.type_index = m_12skline_index;
 	oi.dia_seq = dia.base->_seq;
 	request_trade(ins_id, oi);
 	if (FLAG_OPEN == flag)
 		m_ins_data[ins_id].uuid = oi.uuid;
}

/*
 * 关注5分钟K线
 * class_name ="kavg"
 * name = "均线"
 * param = "5/10/60"
 *
 * 5均线：白色
 * 10均线：浅蓝
 * 60均线：绿色
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
	if (group.dias.end() == group.dias.find(m_12skline_index))
		return;

	for (auto& dia : group.dias[m_12skline_index])
		go(group.ins_id, group.depth.get(), dia);
}

void five_minute_kline_1::go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia) {
	MarketAnalyseDataBase *base = dia.base;
	MarketAnalyseTagBase *five_avg = dia.tags[m_5kavg_index];
	MarketAnalyseTagBase *ten_avg = dia.tags[m_10kavg_index];
	MarketAnalyseTagBase *sixty_avg = dia.tags[m_60kavg_index];

	if (m_ins_data.end() == m_ins_data.find(ins_id))
		m_ins_data[ins_id] = ins_data();
	auto& data = m_ins_data[ins_id];

	if (!five_avg || !ten_avg || !sixty_avg)
		return;

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
			print_thread_safe("[fmk1 ins=%s]5均线向下穿越10均线 %lld %lld\n", ins_id.c_str(), five_avg->_value, ten_avg->_value);
			data.m_five_greater_than_ten = false;		//发生翻转，5向下穿越10
		} else {
			return;		//状态未改变，忽略这个包
		}
	} else {		//5 < 10
		if (five_avg->_value > ten_avg->_value) {		//5 > 10
			printf("[fmk1 ins=%s]5均线向上穿越10均线 %lld %lld\n", ins_id.c_str(), five_avg->_value, ten_avg->_value);
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
				print_thread_safe("[fmk1 ins=%s]60均线在最上面，5均线向下穿越10，卖空!\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_SELL);
			} else {
				return;		//还未开仓，无法平仓
			}
		} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
			if (true == data.m_five_greater_than_ten) {		//5向上穿越10，买多
				data.m_status = STATUS_IN_OPEN_BUY;
				print_thread_safe("[fmk1 ins=%s]60均线在最下面，5均线向上穿越10，买多!\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_BUY);
			} else {
				return;		//还未开仓，无法平仓
			}
		}
	} else {		//之前已经有过开仓平仓操作
		if (STATUS_IN_OPEN_BUY == data.m_status || STATUS_IN_OPEN_SELL == data.m_status) {
			//已经开仓，要平仓，此时只需要关注5和10均线是否发生反转
			if (STATUS_IN_OPEN_BUY == data.m_status) {		//买多
				data.m_status = STATUS_IN_CLOSE_SELL;
				print_thread_safe("[fmk1 ins=%s]已经开仓，之前做的是多仓，现在平仓！\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_SELL);
			} else if (STATUS_IN_OPEN_SELL == data.m_status) {		//卖空
				data.m_status = STATUS_IN_CLOSE_BUY;
				print_thread_safe("[fmk1 ins=%s]已经开仓，之前做的是空仓，现在平仓！\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_BUY);
			}
			return;
		}

		//还未开仓，要开仓，此时需要关注60均线的位置
		if (sixty_avg->_value > five_avg->_value && sixty_avg->_value > ten_avg->_value) {
			//60均线在最上面
			if (false == data.m_five_greater_than_ten) {		//5向下穿越10，卖空
				data.m_status = STATUS_IN_OPEN_SELL;
				print_thread_safe("[fmk1 ins=%s]60均线在最上面，5均线向下穿越10，卖空!\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_SELL);
			}
		} else if (sixty_avg->_value < five_avg->_value && sixty_avg->_value < ten_avg->_value) {
			//60均线在最下面
			if (true == data.m_five_greater_than_ten) {		//5向上穿越10，买多
				data.m_status = STATUS_IN_OPEN_BUY;
				print_thread_safe("[fmk1 ins=%s]60均线在最下面，5均线向上穿越10，买多!\n", ins_id.c_str());
				execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_BUY);
			}
		}
	}
}
