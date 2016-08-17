#include "rollbbtall_ver2.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<rollbbtall_ver2>();
}

void rollbbtall_ver2::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
}

void rollbbtall_ver2::init() {
	m_kline_idx = m_type_map[m_kline].type_index;
}

void rollbbtall_ver2::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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

bool rollbbtall_ver2::seq_outdate(dia_group& dia, ins_data& data) {
	wd_seq uni_seq;		//去掉已过时的K线
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_kseq.vir_seq)
		return true;
	return false;
}

void rollbbtall_ver2::cons_base_price(dia_group& dia, ins_data& data, bool open) {
	if (dia.base->_state != 2)		//初始化时构造的指标需要用到上一根已完成的K线
		return;

	if (seq_outdate(dia, data))
		return;

	__int64 H = dia.ext->max_item.price;		//昨日最高价
	__int64 C = dia.ext->close_item.price;	//昨日收盘价
	__int64 L = dia.ext->min_item.price;		//昨日最低价
	__int64 P = (H+C+L)/3.0;

	data.B_break = H+2*(P-L);
	data.S_setup = P+H-L;
	data.S_enter = 2*P-L;

	data.B_enter = 2*P-H;
	data.B_setup = P-H+L;
	data.S_break = L-2*(H-P);

	data.last_kseq.date = dia.base->_date;
	data.last_kseq.seq = dia.base->_seq;
	if (!open)
		data.sts = STS_PRICE_READY;
	print_thread_safe("[cons_base_price date=%d seq=%d]基准价格构造完成：H：%d，C：%d，L：%d，P：%d，B_break：%d，"
			"S_setup：%d，S_enter：%d，B_enter：%d，B_setup：%d，S_break：%d\n", data.last_kseq.date, data.last_kseq.seq,
			H, C, L, P, data.B_break, data.S_setup, data.S_enter, data.B_enter, data.B_setup, data.S_break);
}

void rollbbtall_ver2::try_open_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	bool open = false;
	if (depth->base.nPrice > data.B_break) {		//价格超过B_break		开多
		data.sts = STS_OPEN_BUY;
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
		print_thread_safe("[try_open_position date=%d seq=%d]最新价（%d）超过B_break（%d），开多仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.B_break);
		open = true;
	}

	if (depth->base.nPrice < data.S_break) {			//价格跌破S_break		开空
		data.sts = STS_OPEN_SELL;
		exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
		print_thread_safe("[try_open_position date=%d seq=%d]最新价（%d）跌破S_break（%d），开空仓！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.S_break);
		open = true;
	}

	if (dia.base->_state == 2) {		//当前K线完成过程中未发生开仓行为，重新计算一系列价格指标
		if (!open)
			print_thread_safe("[try_open_position date=%d seq=%d]当前K线完成过程中未发生开仓行为，重新计算价格指标！\n",
					dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, open);
	}
}

void rollbbtall_ver2::check_price_tendency(MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	//在多仓情况下，最新价小于S_setup
	if (STS_OPEN_BUY == data.sts && depth->base.nPrice < data.S_setup) {
		data.sts = STS_FL_S_SETUP;
		print_thread_safe("[check_price_tendency date=%d seq=%d]在平（多）仓过程中，最新价（%d）回落到S_setup（%d）\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.S_setup);
		return;
	}

	//在空仓情况下，最新价大于B_setup
	if (STS_OPEN_SELL == data.sts && depth->base.nPrice > data.B_setup) {
		data.sts = STS_GU_B_SETUP;
		print_thread_safe("[check_price_tendency date=%d seq=%d]在平（空）仓过程中，最新价（%d）上升到B_setup（%d）\n",
				dia.base->_date, dia.base->_seq, dia.ext->min_item.price, data.B_setup);
		return;
	}

	if (dia.base->_state == 2) {		//记录最新的已完成的K线
		print_thread_safe("[try_open_position date=%d seq=%d]当前K线完成过程中最新价未在S_setup/B_setup这两个指标处发生突破，重新计算价格指标！\n",
				dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, true);
	}
}

void rollbbtall_ver2::try_close_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	//最新价继续回落到S_enter，反手做空
	bool close = false;
	if (STS_FL_S_SETUP == data.sts) {
		if (depth->base.nPrice < data.S_enter) {
			print_thread_safe("[try_close_position date=%d seq=%d]在平（多）仓过程中，最新价（%d）继续回落到S_enter（%d）反手做空！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.S_enter);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
			data.sts = STS_CLOSE_FL_B_SET;
			close = true;
		} else if (depth->base.nPrice > data.S_setup) {
			print_thread_safe("[try_close_position] date=%d seq=%d]在平（多）仓过程中，最新价（%d）再次上升到S_setup（%d）！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.S_setup);
			data.sts = STS_CLOSE_GU_S_SET;
		}
	}

	//最新价继续上升到B_enter，反手做多
	if (STS_GU_B_SETUP == data.sts) {
		if (depth->base.nPrice > data.B_enter) {
			print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，最新价（%d）继续上升到B_enter（%d）反手做多！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.B_enter);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
			data.sts = STS_CLOSE_GU_S_SET;
			close = true;
		} else if (depth->base.nPrice < data.B_setup) {
			print_thread_safe("[try_close_position date=%d seq=%d]在平（空）仓过程中，最新价（%d）再次回落到B_setup（%d）！\n",
					dia.base->_date, dia.base->_seq, depth->base.nPrice, data.B_setup);
			data.sts = STS_CLOSE_FL_B_SET;
		}
	}

	if (dia.base->_state == 2) {		//记录最新的已完成的K线
		if (!close) {
			print_thread_safe("[try_close_position date=%d seq=%d]当前K线完成过程中存在没有平掉的仓位，重新计算价格指标！\n",
					dia.base->_date, dia.base->_seq);
			if (STS_FL_S_SETUP == data.sts)		//状态回退，因为在持仓情况下平仓的两个条件是需要在同一日内同时满足的
				data.sts = STS_OPEN_BUY;
			else		//STS_GU_B_SETUP == sts
				data.sts = STS_OPEN_SELL;
		}
		cons_base_price(dia, data, true);
	}
}

void rollbbtall_ver2::until_price_btop_again(MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	bool bt = false;
	if (STS_CLOSE_GU_S_SET == data.sts && depth->base.nPrice > data.B_break) {
		data.sts = STS_OPEN_BUY;
		print_thread_safe("[until_price_btop_again date=%d seq=%d]当前K线最新价（%d）再一次超过B_break（%d）！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.B_break);
		bt = true;
	}

	if (STS_CLOSE_FL_B_SET == data.sts && depth->base.nPrice < data.S_break) {
		data.sts = STS_OPEN_SELL;
		print_thread_safe("[until_price_btop_again date=%d seq=%d]当前K线最新价（%d）再一次低于S_break（%d）！\n",
				dia.base->_date, dia.base->_seq, depth->base.nPrice, data.S_break);
		bt = true;
	}

	if (dia.base->_state == 2) {		//取到已完成K线重新计算价格指标
		if (!bt)
			print_thread_safe("[until_price_btop_again date=%d seq=%d]当前K线在完成过程中没有回到开仓点的状态，重新"
					"计算价格指标！\n", dia.base->_date, dia.base->_seq);
		cons_base_price(dia, data, true);
	}
}

void rollbbtall_ver2::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_kline_idx))
		return;

	for (auto& dia : group.dias[m_kline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void rollbbtall_ver2::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
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
		check_price_tendency(depth, dia,data);
		break;
	}
	case STS_FL_S_SETUP:
	case STS_GU_B_SETUP: {
		try_close_position(id, depth, dia, data);
		break;
	}
	case STS_CLOSE_GU_S_SET:
	case STS_CLOSE_FL_B_SET: {
		until_price_btop_again(depth, dia, data);
		break;
	}
	}
}
