#include "maxvol5min.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<maxvol5min>();
}

void maxvol5min::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
	m_close_time = atoi(conf["close_time"]);
}

void maxvol5min::init() {
	m_kline_idx = m_type_map[m_kline].type_index;
}

void maxvol5min::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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

bool maxvol5min::seq_outdate(dia_group& dia, ins_data& data) {
	wd_seq uni_seq;		//去掉已过时的K线
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_kseq.vir_seq)
		return true;
	return false;
}

bool maxvol5min::determ_maxvol(dia_group& dia, ins_data& data, bool sts_trans) {
	if (dia.base->_state != 2)
		return false;		//选取成交量最大的K线时取的是已完成的K线

	if (seq_outdate(dia, data))
		return false;		//过时的直接扔掉

	if (dia.base->_date != data.last_kseq.date)
		data.volume_tick = -1;

	bool change = false;
	if (dia.base->_volume_tick > data.volume_tick) {
		change = true;
		data.volume_tick = dia.base->_volume_tick;
		data.max_price = dia.ext->max_item.price;
		data.min_price = dia.ext->min_item.price;
		if (sts_trans)
			data.sts = STS_VOL_DETERM;
		print_thread_safe("[determ_maxvol date=%d seq=%d]成交量最大的K线（记为A）发生更新，成交量=%d，最高价=%d，最低价=%d\n",
				dia.base->_date, dia.base->_seq, data.volume_tick, data.max_price, data.min_price);
	}

	data.last_kseq.date = dia.base->_date;
	data.last_kseq.seq = dia.base->_seq;
	return change;
}

void maxvol5min::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;		//开仓时只关心最新价

	if (depth->base.nPrice > data.max_price) {
		//最新价突破A的最高点，做多
		data.sts = STS_OPEN_BUY;
		data.stop_price = data.min_price;		//最低点设为止损价
		data.last_price = -1;
		print_thread_safe("[try_open_position date=%d seq=%d]当前价格（%d）突破K线A的最高点（%d），开多仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.max_price);
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
	}

	if (depth->base.nPrice < data.min_price) {
		//最新价突破A的最低点，做空
		data.sts = STS_OPEN_SELL;
		data.stop_price = data.max_price;		//最高点设为止损价
		data.last_price = -1;
		print_thread_safe("[try_open_position date=%d seq=%d]当前价格（%d）突破K线A的最低点（%d），开空仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.min_price);
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
	}

	if (dia.base->_state == 2) {		//在尝试开仓过程中取到一根已完成K线，再次比较成交量大小并更新最高价最低价
		print_thread_safe("[try_open_position date=%d seq=%d]在尝试开仓过程中取到一根已完成K线，再次计算成交量最大的K线！\n",
				dia.base->_date, dia.base->_seq);
		determ_maxvol(dia, data, false);
	}
}

void maxvol5min::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;		//平仓也只需要最新价

	bool close = false;
	int64_t tick = m_ins_info[id].price_tick;
	tick = 100000;
	if (STS_OPEN_BUY == data.sts) {
		//平多仓，最新价低于最低价构成的支撑线时止损
		if (depth->base.nPrice < data.stop_price) {
			close = true;
			print_thread_safe("[try_close_position date=%d seq=%d]在平（多）仓过程中，当前价格（%d）低于止损价（%d），止损！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.stop_price);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			data.sts = STS_VOL_DETERM;
		}

		//未发生平仓，检测当前价是否高过A的最高价+10个tick
		if (-1 == data.last_price && depth->base.nPrice > data.max_price+10*tick) {
			data.stop_price = data.max_price;
			data.last_price = depth->base.nPrice;
			print_thread_safe("[try_close_position date=%d seq=%d]在平（多）仓过程中，最新价（%d）高过A的最高价（%d）+10个tick（%d），止损价（%d）"
					"首次更新，记录价格=%d！\n", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.max_price, tick, data.stop_price, data.last_price);
		}
	}

	if (STS_OPEN_SELL == data.sts) {
		//平空仓，最新价高于最高价构成的阻力线时止损
		if (depth->base.nPrice > data.stop_price) {
			close = true;
			print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，当前价格（%d）超过止损价（%d），止损！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.stop_price);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			data.sts = STS_VOL_DETERM;
		}

		//未发生平仓，检测当前价是否低于A的最低价-10个tick
		if (-1 == data.last_price && depth->base.nPrice < data.min_price-10*tick) {
			data.stop_price = data.min_price;
			data.last_price = depth->base.nPrice;
			print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，最新价（%d）小于A的最低价（%d）-10个tick（%d），止损价（%d）"
					"首次更新！\n", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.min_price, tick, data.stop_price);
		}
	}

	if (!close && depth->base.nTime >= m_close_time) {
		print_thread_safe("[try_close_position date=%d time=%d]即将收盘，在收盘前强制平仓！\n", depth->base.nActionDay, depth->base.nTime);
		if (STS_OPEN_BUY == data.sts)
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		else		//STS_OPEN_SELL == data.sts
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		data.sts = STS_INIT;
		return;
	}

	check_10tick_amplitude(depth, dia, data, tick);
	if (dia.base->_state == 2) {		//在尝试平仓过程中取到一根已完成K线，比较成交量大小并更新最高/低价
		print_thread_safe("[try_close_position date=%d seq=%d]在平仓过程中取到一根已完成K线，再次计算成交量最大的K线！\n",
				dia.base->_date, dia.base->_seq);
		if (determ_maxvol(dia, data, false)) {
			if (STS_OPEN_BUY == data.sts)
				data.stop_price = data.min_price;
			else		//STS_OPEN_SELL == data.sts
				data.stop_price = data.max_price;
		}
	}
}

void maxvol5min::check_10tick_amplitude(MarketDepthData *depth, dia_group& dia, ins_data& data, int64_t tick) {
	if (data.last_price == -1)		//说明此前止损价还未调整，只有经过一次调整之后，才能在这里继续调整
		return;

	if (STS_OPEN_BUY == data.sts && depth->base.nPrice == data.last_price+10*tick) {
		//多仓情况下，每上涨10个点，止损上涨5个点
		data.stop_price += 5*tick;
		print_thread_safe("[check_10tick_amplitude date=%d seq=%d]最新价（%d）比上一次更新止损价时记录的价格（%d）上涨了10个tick（%d），止损价"
				"更改为%d！\n", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.last_price, tick, data.stop_price);
		data.last_price = depth->base.nPrice;
	}

	if (STS_OPEN_SELL == data.sts && depth->base.nPrice == data.last_price-10*tick) {
		//空仓情况下，每下降10个点，止损下降5个点
		data.stop_price -= 5*tick;
		print_thread_safe("[check_10tick_amplitude date=%d seq=%d]最新价（%d）比上一次更新止损价时记录的价格（%d）下跌了10个tick（%d），止损价"
				"更改为%d！\n", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.last_price, tick, data.stop_price);
		data.last_price = depth->base.nPrice;
	}
}

void maxvol5min::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_kline_idx))
		return;

	for (auto& dia : group.dias[m_kline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void maxvol5min::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		determ_maxvol(dia, data, true);
		break;
	}
	case STS_VOL_DETERM: {
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
