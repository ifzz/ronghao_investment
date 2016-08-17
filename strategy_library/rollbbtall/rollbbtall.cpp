#include "rollbbtall.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<rollbbtall>();
}

void rollbbtall::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
}

void rollbbtall::init() {
	m_kline_idx = m_type_map[m_kline].type_index;
}

void rollbbtall::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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

bool rollbbtall::seq_outdate(dia_group& dia, ins_data& data) {
	wd_seq uni_seq;		//去掉已过时的K线
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_kseq.vir_seq)
		return true;
	return false;
}

void rollbbtall::cons_base_price(dia_group& dia, ins_data& data, bool open) {
	if (dia.base->_state != 2)		//初始化时构造的反转/突破价格需要用到上一根已完成的K线
		return;

	if (seq_outdate(dia, data))
		return;

	data.ob_sell_price = dia.ext->max_item.price+0.35*(dia.ext->close_item.price-dia.ext->min_item.price);		//昨高+0.35×（昨收-昨低）
	data.rb_sell_price = (1.07/2)*(dia.ext->max_item.price+dia.ext->min_item.price)-0.07*dia.ext->min_item.price;		//（1.07/2）×（昨高+昨低）-0.07×昨低
	data.rb_buy_price = (1.07/2)*(dia.ext->max_item.price+dia.ext->min_item.price)-0.07*dia.ext->max_item.price;		//（1.07/2）×（昨高+昨低）-0.07×昨高

	data.ob_buy_price = dia.ext->min_item.price-0.35*(dia.ext->max_item.price-dia.ext->close_item.price);		//昨低-0.35×（昨高-昨收）
	data.bt_buy_price = data.ob_sell_price+0.25*(data.ob_sell_price-data.ob_buy_price);		//观察卖出价+0.25×（观察卖出价-观察买入价）
	data.bt_sell_price = data.ob_buy_price-0.25*(data.ob_sell_price-data.ob_buy_price);		//观察买入价-0.25×（观察卖出价-观察买入价）

	data.last_kseq.date = dia.base->_date;
	data.last_kseq.seq = dia.base->_seq;
	if (!open)
		data.sts = STS_PRICE_READY;
	print_thread_safe("[cons_base_price date=%d seq=%d]基准价格构造完成：观察卖出价：%d，反转卖出价：%d，"
			"反转买入价：%d，观察买入价：%d，突破买入价：%d，突破卖出价：%d\n", data.last_kseq.date, data.last_kseq.seq,
			data.ob_sell_price, data.rb_sell_price, data.rb_buy_price, data.ob_buy_price, data.bt_buy_price, data.bt_sell_price);
}

void rollbbtall::try_open_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	if (depth->base.nPrice > data.bt_buy_price) {		//价格超过突破买入价		开多
		data.sts = STS_OPEN_BUY;
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
		print_thread_safe("[try_open_position date=%d seq=%d]最新价（%d）超过突破买入价（%d），开多仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.bt_buy_price);
		return;
	}

	if (depth->base.nPrice < data.bt_sell_price) {			//价格跌破突破卖出价		开空
		data.sts = STS_OPEN_SELL;
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
		print_thread_safe("[try_open_position date=%d seq=%d]最新价（%d）跌破突破卖出价（%d），开空仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.bt_sell_price);
		return;
	}

	if (dia.base->_state == 2) {		//当前K线完成过程中未发生开仓行为，重新计算一系列价格指标
		print_thread_safe("[try_open_position date=%d seq=%d]当前K线完成过程中未发生开仓行为，重新计算价格指标！\n",
				dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, false);
	}
}

void rollbbtall::check_price_tendency(MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	//当日最高价超过观察卖出价
	if (STS_OPEN_BUY == data.sts && dia.ext->max_item.price > data.ob_sell_price) {
		data.sts = STS_GT_OB_SELL;
		print_thread_safe("[check_price_tendency date=%d seq=%d]在平（多）仓过程中，日内最高价（%d）超过观察卖出价（%d）\n",
				dia.base->_date, dia.base->_seq, dia.ext->max_item.price, data.ob_sell_price);
		return;
	}

	//当日最低价低于观察买入价
	if (STS_OPEN_SELL == data.sts && dia.ext->min_item.price < data.ob_buy_price) {
		data.sts = STS_LT_OB_BUY;
		print_thread_safe("[check_price_tendency date=%d seq=%d]在平（空）仓过程中，日内最低价（%d）低于观察买入价（%d）\n",
				dia.base->_date, dia.base->_seq, dia.ext->min_item.price, data.ob_buy_price);
		return;
	}

	if (dia.base->_state == 2) {		//记录最新的已完成的K线
		print_thread_safe("[try_open_position date=%d seq=%d]当前K线完成过程中日内最高/低价未发生突破，重新计算价格指标！\n",
				dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, true);
	}
}

void rollbbtall::try_close_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	if (STS_GT_OB_SELL == data.sts && dia.ext->max_item.price < data.rb_sell_price) {
		//日内最高价进一步跌破反转卖出价构成的支撑线后，反手做空
		print_thread_safe("[try_close_position date=%d seq=%d]在平（多）仓过程中，盘中价格回落，日内最高价（%d）进一步"
				"跌破反转卖出价（%d）构成的支撑线，反手做空！\n", dia.base->_date, dia.base->_seq, dia.ext->max_item.price,
				data.rb_sell_price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
		data.sts = STS_OPEN_SELL;
		return;
	}

	if (STS_LT_OB_BUY == data.sts && dia.ext->min_item.price > data.rb_buy_price) {
		//日内最低价进一步超过反转买入价构成的阻力线后，反手做多
		print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，盘中价格反弹，日内最低价（%d）进一步"
				"超过反转买入价（%d）构成的阻力线，反手做多！\n", dia.base->_date, dia.base->_seq, dia.ext->min_item.price,
				data.rb_buy_price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
		data.sts = STS_OPEN_BUY;
		return;
	}

	if (dia.base->_state == 2) {		//记录最新的已完成的K线
		print_thread_safe("[try_open_position date=%d seq=%d]当前K线完成过程中存在没有平掉的仓位，重新计算价格指标！\n",
				dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, true);

		if (STS_GT_OB_SELL == data.sts)		//状态回退，因为在持仓情况下平仓的两个条件是需要在同一日内同时满足的
			data.sts = STS_OPEN_BUY;
		else		//STS_LT_OB_BUY == sts
			data.sts = STS_OPEN_SELL;
	}
}

void rollbbtall::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_kline_idx))
		return;

	for (auto& dia : group.dias[m_kline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void rollbbtall::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		cons_base_price(dia, data, false);
		break;
	}
	case STS_PRICE_READY: {
		try_open_position(id, depth, dia, data);
		break;
	}
	case STS_OPEN_BUY:
	case STS_OPEN_SELL: {
		check_price_tendency(depth, dia, data);
		break;
	}
	case STS_GT_OB_SELL:
	case STS_LT_OB_BUY: {
		try_close_position(id, depth, dia, data);
		break;
	}
	}
}
