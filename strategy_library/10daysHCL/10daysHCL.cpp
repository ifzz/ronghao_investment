#include "10daysHCL.h"
#include <algorithm>

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<days10HCL>();
}

void days10HCL::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
	m_close_time = atoi(conf["close_time"]);
}

void days10HCL::init() {
	m_kline_idx = m_type_map[m_kline].type_index;
}

void days10HCL::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
		OFFSET_FLAG flag, TRADE_DIRECTION dir) {
	datetime  mdt;
	mdt.date = depth->base.nActionDay;
	mdt.time = depth->base.nTime;

	order_instruction oi;
	if (FLAG_CLOSE == flag)
		oi.uuid = m_ins_data[id].uuid;
	oi.market = mdt;
	oi.flag = flag;
	oi.direction = dir;
	oi.price = depth->base.nPrice;		//最新价下单

	oi.vol_cnt = 1;
	oi.level = 1;

	oi.dia_seq = dia.base->_seq;
	oi.type_index = m_kline_idx;
	request_trade(id, oi);
	if (FLAG_OPEN == flag)
		m_ins_data[id].uuid = oi.uuid;
}

bool days10HCL::seq_outdate(dia_group& dia, ins_data& data) {
	wd_seq uni_seq;		//去掉已过时的K线
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_kseq.vir_seq)
		return true;
	return false;
}

void days10HCL::cons_10days_hcl(dia_group& dia, ins_data& data) {
	if (dia.base->_state != 2)
		return;		//构造的前10根日线都必须是已经完成的

	if (seq_outdate(dia, data))
		return;		//如果来的是一个过时的包，直接丢弃

	if (10 == data.day10slist.size())
		data.day10slist.pop_front();

	__int64 H_minus_C = dia.ext->max_item.price-dia.ext->close_item.price;
	__int64 C_minus_L = dia.ext->close_item.price-dia.ext->min_item.price;
	__int64 min = std::min(H_minus_C, C_minus_L);
	data.day10slist.push_back(min);
	data.last_kseq.date = dia.base->_date;
	data.last_kseq.seq = dia.base->_seq;
	print_thread_safe("[cons_10days_hcl date=%d seq=%d]取到一根已完成的K线 H=%d，C=%d，L=%d，H-C=%d，C-L=%d，min=%d\n",
			dia.base->_date, dia.base->_seq, dia.ext->max_item.price, dia.ext->close_item.price, dia.ext->min_item.price, H_minus_C, C_minus_L, min);

	if (10 == data.day10slist.size()) {
		__int64 sigma_10 = std::accumulate(data.day10slist.begin(), data.day10slist.end(), 0);
		data.A = sigma_10/10;
		print_thread_safe("[cons_10days_hcl date=%d seq=%d]已取满10根满足条件的K线，这10根K线中较小值的总和=%d，平均值A=%d\n",
				dia.base->_date, dia.base->_seq, sigma_10, data.A);
		data.sts = STS_10KLINE_READY;
	}
}

void days10HCL::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	//这个策略在K线完成时将产生平仓操作，所以在完成时不再开仓
	if (dia.base->_state != 2) {
		if (depth->base.nPrice > dia.ext->open_item.price+data.A) {
			//最新价大于当天开盘价+A，开多仓
			data.sts = STS_OPEN_BUY;
			print_thread_safe("[try_open_position date=%d seq=%d]当前最新价（%d）大于当天开盘价（%d）+A（%d），开多仓！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, dia.ext->open_item.price, data.A);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
		}

		if (depth->base.nPrice < dia.ext->open_item.price-data.A) {
			//最新价小于当天开盘价-A，开空仓
			data.sts = STS_OPEN_SELL;
			print_thread_safe("[try_open_position date=%d seq=%d]当前最新价（%d）小于当天开盘价（%d）-A（%d），开空仓！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, dia.ext->open_item.price, data.A);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
		}
		return;
	}

	//未返回，说明此处这根K线处于完成状态
	print_thread_safe("[try_open_position date=%d seq=%d]在尝试开仓过程中未找到开仓点，现取到一根已完成K线，重新构造10根K线！\n",
			dia.base->_date, dia.base->_seq);
	cons_10days_hcl(dia, data);
}

void days10HCL::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	//在K线完成过程中以止损方式平仓
	if (STS_OPEN_BUY == data.sts && depth->base.nPrice < dia.ext->open_item.price) {
		//在多仓情况下，最新价小于开盘价，止损
		print_thread_safe("[try_close_position date=%d seq=%d]在平（多）仓过程中，当前最新价（%d）小于开盘价（%d），止损！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, dia.ext->open_item.price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		data.sts = STS_10KLINE_READY;
		return;
	}

	if (STS_OPEN_SELL == data.sts && depth->base.nPrice > dia.ext->open_item.price) {
		//在空仓情况下，最新价大于开盘价，止损
		print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，当前最新价（%d）大于开盘价（%d），止损\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, dia.ext->open_item.price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		data.sts = STS_10KLINE_READY;
		return;
	}

	//未返回，说明这根K线处于完成状态
	if (dia.base->_time > m_close_time) {
		if (STS_OPEN_BUY == data.sts) {
			print_thread_safe("[try_close_position date=%d seq=%d]当前时间为%d，收盘平（多）仓！\n",
					dia.base->_date, dia.base->_seq, dia.base->_time);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		} else {		//STS_OPEN_SELL == data.sts
			print_thread_safe("[try_close_position date=%d seq=%d]当前时间为%d，收盘平（空）仓！\n",
					dia.base->_date, dia.base->_seq, dia.base->_time);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		}
		data.sts = STS_INIT;
	}
}

void days10HCL::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_kline_idx))
		return;

	for (auto& dia : group.dias[m_kline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void days10HCL::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		cons_10days_hcl(dia, data);
		break;
	}
	case STS_10KLINE_READY: {
		try_open_position(id, depth, dia, data);
		break;
	}
	case STS_OPEN_BUY:
	case STS_OPEN_SELL: {
		try_close_position(id, depth, dia, data);
		break;
	}
	}
}
