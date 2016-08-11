#include "strategy_demo4.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<strategy_demo4>();
}

void strategy_demo4::init() {
	m_1minkline_index = m_type_map["1分钟kline"].type_index;
}

 /*
  * 这个策略只关注1分钟K线
  * ①连续取n（n>=10）根K线，这些K线都满足其比率（振幅/收盘）<= 0.5%
  * ②在第n+1根K线的振幅超过1%之后，进一步若有第n+2根K线的最新价超过n+1的收盘价，则产生开仓行为
  * 		->当其最新价高于收盘价时，做多
  * 		->最新价低于收盘价时，做空
  * 产生开仓行为之后，满足以下条件进行平仓，以做多为例说明其平仓条件：
  * 		->若当前这根K线的最新价低于n+2根K线的最低价，则立即平仓（止损）
  * 		->若当前这根K线的最新价跌破其开盘价的0.5%，则立即平仓（止盈）
  * 	做空的平仓条件与做多相反
  */

void strategy_demo4::fi_init(dia_group& dia, ins_data& data) {
	if (2 != dia.base->_state)
		return;

	if (dia.base->_seq > data.m_seq+1)		//非连续的话就要重新选取了
		data.m_init_k_num = 0;

	//初始化时选取的N根K线必定都处于完成状态，并且要保证是连续的
	float ratio = (dia.ext->max_item.price-dia.ext->min_item.price)*1.0f/dia.ext->close_item.price;
	if (ratio <= 0.005)		//比率满足<=0.5%
		data.m_init_k_num++;
	else		//遇到一根不满足状态的K线
		data.m_init_k_num = 0;

	if (data.m_init_k_num >= 10) {
		data.m_sts = STATUS_N_READY;
		print_thread_safe("[strategy_demo4]已经取到满足状态的连续%d根K线！\n", data.m_init_k_num);
	}
}

void strategy_demo4::fi_n_ready(dia_group& dia, ins_data& data) {
	//同样的第n+1根K线也必须是完成状态的，否则无法判断其振幅是否满足要求
	if (2 != dia.base->_state)
		return;

	if (dia.base->_seq > data.m_seq+1) {
		data.m_sts = STATUS_INIT;		//此时处于非连续状态，重新选
		data.m_init_k_num = 0;
		return;
	}

	float ratio = (dia.ext->max_item.price-dia.ext->min_item.price)*1.0f/dia.ext->close_item.price;
	if (ratio >= 0.01) {		//取到的第N+1根K线满足要求
		data.m_sts = STATUS_N_PLUS_ONE_READY;
		data.m_n_plus_one_seq = dia.base->_seq;
		data.m_n_plus_one_close_price = dia.ext->close_item.price;
		print_thread_safe("[strategy_demo4]取到的当前这根K线，其振幅高于1%，最高价：%d，最低级：%d，收盘价：%d，"
				"准备进入开仓状态\n", dia.ext->max_item.price, dia.ext->min_item.price, dia.ext->close_item.price);
	} else if (ratio <= 0.005) {		//振幅仍然满足 <= 0.05%
		data.m_sts = STATUS_N_READY;
		data.m_init_k_num++;
	} else {
		data.m_sts = STATUS_INIT;		//振幅处于0.5%和1%之间，不满足任何要求，重新选取连续N根K线
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::fi_n_plus_one_ready(const std::string& ins_id, dia_group& dia, ins_data& data) {
	//开仓时并不关心当前K线是否处于完成状态，在完成过程中也可以开仓，但要保证连续
	if (dia.base->_seq > data.m_seq+1 || dia.base->_seq > data.m_n_plus_one_seq+1) {
		data.m_sts = STATUS_INIT;		//此时处于非连续状态，重新选
		data.m_init_k_num = 0;
		return;
	}

	//进一步若有第n+2根K线的最新价超过n+1的收盘价，则产生开仓行为
	if (dia.ext->close_item.price > data.m_n_plus_one_close_price) {
		//最新价高于收盘价时，做多
		data.m_sts = STATUS_OPEN_BUY;
		data.m_n_plus_two_lowest_price = dia.ext->min_item.price;		//做多时记录n+2的最低价用以止损

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d超过前一根K线的收盘价：%d，开多仓！\n",
				dia.ext->close_item.price, data.m_n_plus_one_close_price);
		execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_BUY);
	} else if (dia.ext->close_item.price < data.m_n_plus_one_close_price) {
		//最新价低于收盘价时，做空
		data.m_sts = STATUS_OPEN_SELL;
		data.m_n_plus_two_highest_price = dia.ext->max_item.price;		//做空时记录n+2的最高价用来止损

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d跌破前一根K线的收盘价：%d，开空仓！\n",
				dia.ext->close_item.price, data.m_n_plus_one_close_price);
		execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_SELL);
	}
}

void strategy_demo4::fi_open_buy(const std::string& ins_id, dia_group& dia, ins_data& data) {
	//平仓时同样也不关心K线的状态，此时也不用保证其连续性

	if (dia.ext->close_item.price < data.m_n_plus_two_lowest_price) {
		//若当前这根K线的最新价低于n+2根K线的最低价，则立即平仓（止损）
		data.m_sts = STATUS_CLOSE_SELL;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 低于n+2根K线的最低价：%d，止损！\n",
				dia.ext->close_item.price, data.m_n_plus_two_lowest_price);
	}

	//若当前这根K线的最新价跌破其开盘价的0.5%，则立即平仓（止盈）
	if (dia.ext->close_item.price < dia.ext->open_item.price*0.995) {
		data.m_sts = STATUS_CLOSE_SELL;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d，跌破其开盘价：%d的0.5%%，止盈！\n",
				dia.ext->close_item.price, dia.ext->open_item.price);
	}

	if (STATUS_CLOSE_SELL == data.m_sts) {
		execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_SELL);
		data.m_sts = STATUS_INIT;		//重新开始新一轮的交易行为
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::fi_open_sell(const std::string& ins_id, dia_group& dia, ins_data& data) {
	//做空的平仓条件与做多相反
	if (dia.ext->close_item.price > data.m_n_plus_two_highest_price) {
		//若当前这根K线的最新价高于n+2根K线的最高价，立即平仓（止损）
		data.m_sts = STATUS_CLOSE_BUY;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 高于n+2根K线的最高价：%d，止损！\n",
				dia.ext->close_item.price, data.m_n_plus_two_highest_price);
	}

	if (dia.ext->close_item.price > dia.ext->open_item.price*1.005) {
		//当前这根K线的最新价上涨其开盘价的0.5%，则立即平仓（止盈）
		data.m_sts = STATUS_CLOSE_BUY;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 上涨其开盘价：%d的0.5%%，止盈！\n",
				dia.ext->close_item.price, dia.ext->open_item.price);
	}

	if (STATUS_CLOSE_SELL == data.m_sts) {
		execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_BUY);
		data.m_sts = STATUS_INIT;		//重新开始新一轮的交易行为
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::execute_trade(const std::string& ins_id, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
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

 	oi.type_index = m_1minkline_index;
 	oi.dia_seq = dia.base->_seq;
 	request_trade(ins_id, oi);
 	if (FLAG_OPEN == flag)
 		m_ins_data[ins_id].uuid = oi.uuid;
}

void strategy_demo4::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_1minkline_index))
		return;		//只关注一分钟K线

	for (auto& dia : group.dias[m_1minkline_index])
		go(group.ins_id, group.depth.get(), dia);
}

void strategy_demo4::go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia) {
	if (m_ins_data.end() == m_ins_data.find(ins_id))
		m_ins_data[ins_id] = ins_data();
	auto& data = m_ins_data[ins_id];

	if (dia.base->_seq < data.m_seq)		//若当前序列号低于前面的序列号，说明来的是一个tag，直接忽略
		return;

	switch (data.m_sts) {
	case STATUS_INIT:		//初始化状态，还未取够满足状态的连续N根K线
		fi_init(dia, data);
		break;
	case STATUS_N_READY:		//已取满连续的N根K线
		fi_n_ready(dia, data);
		break;
	case STATUS_N_PLUS_ONE_READY:		//第N+1根K线也是满足状态的
		fi_n_plus_one_ready(ins_id, dia, data);
		break;
	case STATUS_OPEN_BUY:			//产生开仓行为，做多
		fi_open_buy(ins_id, dia, data);
		break;
	case STATUS_OPEN_SELL:		//产生开仓行为，做空
		fi_open_sell(ins_id, dia, data);
		break;
	default:
		break;
	}

	if (dia.base->_seq > data.m_seq)		//记录前一根K线的序号
		data.m_seq = dia.base->_seq;
}
