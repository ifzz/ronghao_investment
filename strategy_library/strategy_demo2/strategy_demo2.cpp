#include "strategy_demo2.h"

 std::shared_ptr<strategy_base> create_strategy() {
	 return std::make_shared<strategy_demo2>();
 }

void strategy_demo2::init() {
	m_1minkline_index = m_type_map["1分钟kline"].type_index;
}

void strategy_demo2::execute_trade(const std::string& ins_id, MarketDepthData *depth, dia_group& dia,
		OFFSET_FLAG flag, TRADE_DIRECTION direction) {
	 datetime market;
	 market.date = depth->base.nActionDay;
	 market.time = depth->base.nTime;

	 order_instruction oi;
	 if (FLAG_CLOSE == flag)
		 oi.uuid = m_ins_data[ins_id].uuid;

	 oi.market = market;
	 oi.direction = direction;
	 oi.flag = flag;
	 oi.price = depth->base.nPrice;

	 oi.vol_cnt = 1;
	 oi.level = 1;

	 oi.type_index = m_1minkline_index;
	 oi.dia_seq = dia.base->_seq;
	 request_trade(ins_id, oi);
	 if (FLAG_OPEN == flag)
		 m_ins_data[ins_id].uuid = oi.uuid;
}

 /*
  * 这个策略关注1分钟k线
  * 当前价>开盘价+上一根K线振幅 ->做多（同时平前一个空仓）
  * 当前价<开盘价-上一根K线振幅->做空（同时平前一个多仓）
  */

void strategy_demo2::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_1minkline_index))
		return;		//只关注1分钟K线

	for (auto& dia : group.dias[m_1minkline_index])
		go(group.ins_id, group.depth.get(), dia);
}

void strategy_demo2::go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia) {
	if (m_ins_data.end() == m_ins_data.find(ins_id))
		m_ins_data[ins_id] = ins_data();
	auto& data = m_ins_data[ins_id];

	//当前状态非STATUS_INIT时将有可能产生开仓行为，而开仓总是需要上一根K线的最高/低价
	if (data.m_sts != STATUS_INIT) {
		if (dia.base->_seq > data.m_last_k_line._seq) {
			data.m_last_amplitude = data.m_last_k_line.m_max_price-data.m_last_k_line.m_min_price;
		} else if (dia.base->_seq < data.m_last_k_line._seq) {
			return;		//直接忽略这个包，说明更新了之前data下的一个tag
		}
	}

	switch (data.m_sts) {
	case STATUS_INIT: {		//确定第一根K线
		if (2 != dia.base->_state)
			return;

		data.m_sts = STATUS_READY;		//表示已经处于可以开仓的状态
		break;
	}
	case STATUS_READY: {		//此处产生第一次开仓行为
		if ((depth->base.nPrice > dia.ext->open_item.price+data.m_last_amplitude) ||
				(depth->base.nPrice < dia.ext->open_item.price-data.m_last_amplitude)) {
			if (depth->base.nPrice > dia.ext->open_item.price+data.m_last_amplitude) {		//做多
				execute_trade(ins_id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
				data.m_sts = STATUS_OPEN_BUY;
			} else if (depth->base.nPrice < dia.ext->open_item.price-data.m_last_amplitude) {		//做空
				execute_trade(ins_id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
				data.m_sts = STATUS_OPEN_SELL;
			}
		}
		break;
	}
	case STATUS_OPEN_BUY: {		//买多
		if (depth->base.nPrice < dia.ext->open_item.price-data.m_last_amplitude) {
			//当前最新价小于上一根K线的最低价，卖平并做空
			execute_trade(ins_id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			execute_trade(ins_id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
			data.m_sts = STATUS_OPEN_SELL;
		}
		break;
	}
	case STATUS_OPEN_SELL: {
		if (depth->base.nPrice > dia.ext->open_item.price+data.m_last_amplitude) {
			//当前最新价大于上一根K线的最高价，买平并买多
			execute_trade(ins_id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			execute_trade(ins_id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
			data.m_sts = STATUS_OPEN_BUY;
		}
		break;
	}
	}

	//总是记录上一根K线的序号、最高价和最低价
	data.m_last_k_line._seq = dia.base->_seq;
	data.m_last_k_line.m_max_price = dia.ext->max_item.price;
	data.m_last_k_line.m_min_price = dia.ext->min_item.price;
}
