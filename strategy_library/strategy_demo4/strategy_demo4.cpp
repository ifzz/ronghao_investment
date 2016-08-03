#include "strategy_demo4.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<strategy_demo4>();
}

void strategy_demo4::init() {
	strategy_base::init();
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

void strategy_demo4::fi_init(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, ins_data& data) {
	if (2 != base->_state)
		return;

	if (base->_seq > data.m_seq+1)		//非连续的话就要重新选取了
		data.m_init_k_num = 0;

	//初始化时选取的N根K线必定都处于完成状态，并且要保证是连续的
	float ratio = (ext->max_item.price-ext->min_item.price)*1.0f/ext->close_item.price;
	if (ratio <= 0.005)		//比率满足<=0.5%
		data.m_init_k_num++;
	else		//遇到一根不满足状态的K线
		data.m_init_k_num = 0;

	if (data.m_init_k_num >= 10) {
		data.m_sts = STATUS_N_READY;
		print_thread_safe("[strategy_demo4]已经取到满足状态的连续%d根K线！\n", data.m_init_k_num);
	}
}

void strategy_demo4::fi_n_ready(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, ins_data& data) {
	//同样的第n+1根K线也必须是完成状态的，否则无法判断其振幅是否满足要求
	if (2 != base->_state)
		return;

	if (base->_seq > data.m_seq+1) {
		data.m_sts = STATUS_INIT;		//此时处于非连续状态，重新选
		data.m_init_k_num = 0;
		return;
	}

	float ratio = (ext->max_item.price-ext->min_item.price)*1.0f/ext->close_item.price;
	if (ratio >= 0.01) {		//取到的第N+1根K线满足要求
		data.m_sts = STATUS_N_PLUS_ONE_READY;
		data.m_n_plus_one_seq = base->_seq;
		data.m_n_plus_one_close_price = ext->close_item.price;
		print_thread_safe("[strategy_demo4]取到的当前这根K线，其振幅高于1%，最高价：%d，最低级：%d，收盘价：%d，"
				"准备进入开仓状态\n", ext->max_item.price, ext->min_item.price, ext->close_item.price);
	} else if (ratio <= 0.005) {		//振幅仍然满足 <= 0.05%
		data.m_sts = STATUS_N_READY;
		data.m_init_k_num++;
	} else {
		data.m_sts = STATUS_INIT;		//振幅处于0.5%和1%之间，不满足任何要求，重新选取连续N根K线
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::fi_n_plus_one_ready(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group,
		ins_data& data) {
	//开仓时并不关心当前K线是否处于完成状态，在完成过程中也可以开仓，但要保证连续
	if (base->_seq > data.m_seq+1 || base->_seq > data.m_n_plus_one_seq+1) {
		data.m_sts = STATUS_INIT;		//此时处于非连续状态，重新选
		data.m_init_k_num = 0;
		return;
	}

	//进一步若有第n+2根K线的最新价超过n+1的收盘价，则产生开仓行为
	if (ext->close_item.price > data.m_n_plus_one_close_price) {
		//最新价高于收盘价时，做多
		data.m_sts = STATUS_OPEN_BUY;
		data.m_n_plus_two_lowest_price = ext->min_item.price;		//做多时记录n+2的最低价用以止损

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d超过前一根K线的收盘价：%d，开多仓！\n",
				ext->close_item.price, data.m_n_plus_one_close_price);
		execute_trade(group, FLAG_OPEN, DIRECTION_BUY);
	} else if (ext->close_item.price < data.m_n_plus_one_close_price) {
		//最新价低于收盘价时，做空
		data.m_sts = STATUS_OPEN_SELL;
		data.m_n_plus_two_highest_price = ext->max_item.price;		//做空时记录n+2的最高价用来止损

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d跌破前一根K线的收盘价：%d，开空仓！\n",
				ext->close_item.price, data.m_n_plus_one_close_price);
		execute_trade(group, FLAG_OPEN, DIRECTION_SELL);
	}
}

void strategy_demo4::fi_open_buy(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group,
		ins_data& data) {
	//平仓时同样也不关心K线的状态，此时也不用保证其连续性

	if (ext->close_item.price < data.m_n_plus_two_lowest_price) {
		//若当前这根K线的最新价低于n+2根K线的最低价，则立即平仓（止损）
		data.m_sts = STATUS_CLOSE_SELL;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 低于n+2根K线的最低价：%d，止损！\n",
				ext->close_item.price, data.m_n_plus_two_lowest_price);
	}

	//若当前这根K线的最新价跌破其开盘价的0.5%，则立即平仓（止盈）
	if (ext->close_item.price < ext->open_item.price*0.995) {
		data.m_sts = STATUS_CLOSE_SELL;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d，跌破其开盘价：%d的0.5%%，止盈！\n",
				ext->close_item.price, ext->open_item.price);
	}

	if (STATUS_CLOSE_SELL == data.m_sts) {
		execute_trade(group, FLAG_CLOSE, DIRECTION_SELL);
		data.m_sts = STATUS_INIT;		//重新开始新一轮的交易行为
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::fi_open_sell(MarketAnalyseDataBase *base, MarketAnalyseKline *ext, depth_dia_group& group,
		ins_data& data) {
	//做空的平仓条件与做多相反
	if (ext->close_item.price > data.m_n_plus_two_highest_price) {
		//若当前这根K线的最新价高于n+2根K线的最高价，立即平仓（止损）
		data.m_sts = STATUS_CLOSE_BUY;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 高于n+2根K线的最高价：%d，止损！\n",
				ext->close_item.price, data.m_n_plus_two_highest_price);
	}

	if (ext->close_item.price > ext->open_item.price*1.005) {
		//当前这根K线的最新价上涨其开盘价的0.5%，则立即平仓（止盈）
		data.m_sts = STATUS_CLOSE_BUY;

		print_thread_safe("[strategy_demo4]当前这根K线的最新价：%d 上涨其开盘价：%d的0.5%%，止盈！\n",
				ext->close_item.price, ext->open_item.price);
	}

	if (STATUS_CLOSE_SELL == data.m_sts) {
		execute_trade(group, FLAG_CLOSE, DIRECTION_BUY);
		data.m_sts = STATUS_INIT;		//重新开始新一轮的交易行为
		data.m_init_k_num = 0;
	}
}

void strategy_demo4::execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
 	datetime market;
 	market.date = group.depth->base.nActionDay;
 	market.time = group.depth->base.nTime;

 	order_instruction oi;
 	oi.type_index = m_1minkline_index;
 	oi.dia_seq = group.diagrams[m_1minkline_index].base->_seq;
 	oi.level = 1;
 	oi.market = market;
 	oi.price = group.diagrams[m_1minkline_index].ext->close_item.price;
 	oi.flag = flag;
 	oi.direction = direction;
 	request_trade(group.ins_id, oi);
}

void strategy_demo4::execute(depth_dia_group& group) {
	if (group.diagrams.end() == group.diagrams.find(m_1minkline_index))
		return;		//只关注一分钟K线

	if (m_ins_data.end() == m_ins_data.find(group.ins_id))
		m_ins_data[group.ins_id] = ins_data();

	MarketAnalyseDataBase *base = group.diagrams[m_1minkline_index].base;
	MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	auto& data = m_ins_data[group.ins_id];

	if (base->_seq < data.m_seq)		//若当前序列号低于前面的序列号，说明来的是一个tag，直接忽略
		return;

	switch (data.m_sts) {
	case STATUS_INIT:		//初始化状态，还未取够满足状态的连续N根K线
		fi_init(base, ext, data);
		break;
	case STATUS_N_READY:		//已取满连续的N根K线
		fi_n_ready(base, ext, data);
		break;
	case STATUS_N_PLUS_ONE_READY:		//第N+1根K线也是满足状态的
		fi_n_plus_one_ready(base, ext, group, data);
		break;
	case STATUS_OPEN_BUY:			//产生开仓行为，做多
		fi_open_buy(base, ext, group, data);
		break;
	case STATUS_OPEN_SELL:		//产生开仓行为，做空
		fi_open_sell(base, ext, group, data);
		break;
	default:
		break;
	}

	if (base->_seq > data.m_seq)		//记录前一根K线的序号
		data.m_seq = base->_seq;
}
