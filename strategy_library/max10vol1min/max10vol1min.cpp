#include "max10vol1min.h"
#include <algorithm>

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<max10vol1min>();
}

void max10vol1min::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
	m_close_time = atoi(conf["close_time"]);
}

void max10vol1min::init() {
	m_kline_idx = m_type_map[m_kline].type_index;

	for (auto& ins_info : m_ins_info) {
		auto& data = m_ins_data[ins_info.first];
		for_each_his_dia(ins_info.first, 1, "分钟", "kline", 0, [&](dia_group& dia, void *args)->void {
			data.day_max = std::max(data.day_max, dia.ext->max_item.price);
			data.day_min = std::min(data.day_min, dia.ext->min_item.price);
		}, nullptr);
		print_thread_safe("[init id=%s]当天历史加载完成，当日最高点=%lld，最低点=%lld\n", ins_info.first.c_str(), data.day_max, data.day_min);
	}
}

void max10vol1min::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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

bool max10vol1min::seq_outdate(dia_group& dia, ins_data& data) {
	wd_seq uni_seq;		//去掉已过时的K线
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_kseq.vir_seq)
		return true;
	return false;
}

void max10vol1min::update_dayprice_and_seq(dia_group& dia, ins_data& data) {
	//day price
	data.day_max = std::max(data.day_max, dia.ext->max_item.price);
	data.day_min = std::min(data.day_min, dia.ext->min_item.price);

	//kline seq
	data.last_kseq.date = dia.base->_date;
	data.last_kseq.seq = dia.base->_seq;
}

void max10vol1min::cons_forward_10kline(const std::string& id, dia_group& dia, ins_data& data) {
	if (dia.base->_state != 2)
		return;

	if (seq_outdate(dia, data))
		return;

	if (10 == data.far10k.size())
		data.far10k.pop_front();

	if (dia.base->_date != data.last_kseq.date) {
		//日期发生更新，重新选取前10根K线并设置某些初值
		data.far10k.clear();
		data.day_max = INT64_MIN;
		data.day_min = INT64_MAX;
	}

	//构造前10根K线
	__int64 max = dia.ext->max_item.price;
	__int64 min = dia.ext->min_item.price;
	__int64 vol = dia.base->_volume_tick;
	data.far10k.push_back({max, min, vol});
	update_dayprice_and_seq(dia, data);
	print_thread_safe("[cons_forward_10kline date=%d seq=%d id=%s]取到一根已完成K线，最高价=%lld，最低价=%lld，成交量=%lld，并且当日最高价=%lld，"
			"当日最低价=%lld\n", dia.base->_date, dia.base->_seq, id.c_str(), max, min, vol, data.day_max, data.day_min);

	if (10 == data.far10k.size()) {
		//构造完成，计算这10根K线中成交量最大的那根K线
		data.mvol_it = std::max_element(data.far10k.begin(), data.far10k.end(),
				[](const far10kline& i, const far10kline& j)->bool { return i.vol_tick < j.vol_tick; });
		data.sts = STS_10KLINE_READY;
		print_thread_safe("[cons_forward_10kline date=%d seq=%d id=%s]已取满10根K线，计算出成交量最大的K线，其最高价=%lld，最低价=%lld，"
				"成交量=%lld\n", dia.base->_date, dia.base->_seq, id.c_str(), data.mvol_it->max_price, data.mvol_it->min_price, data.mvol_it->vol_tick);
	}
}

void max10vol1min::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))		//过时的K线直接扔掉
		return;

	bool open = false;
	if (dia.base->_volume_tick > data.mvol_it->vol_tick) {
		//当前K线的成交量大于前10根K线中成交量最大的那一根
		if (depth->base.nPrice > data.day_max) {		//最新价突破当日最高价，开多
			open = true;
			data.sts = STS_OPEN_BUY;
			print_thread_safe("[try_open_position date=%d seq=%d id=%s]当前成交量（%d）大于前10根K线的最大成交量（%d），并且当前最新价（%lld）"
					"大于当日最高价（%lld），开多！\n", dia.base->_date, dia.base->_seq, id.c_str(), dia.base->_volume_tick, data.mvol_it->vol_tick,
					depth->base.nPrice, data.day_max);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
		} else if (depth->base.nPrice < data.day_min) {		//最新价突破当日最低价，开空
			open = true;
			data.sts = STS_OPEN_SELL;
			print_thread_safe("[try_open_position date=%d seq=%d id=%s]当前成交量（%d）大于前10根K线的最大成交量（%d），并且当前最新价（%lld）"
					"小于当日最低价（%lld），开空！\n", dia.base->_date, dia.base->_seq, id.c_str(), dia.base->_volume_tick, data.mvol_it->vol_tick,
					depth->base.nPrice, data.day_min);
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
		}
		if (open) {
			data.open_price = depth->base.nPrice;
			data.get_opvol_and_kline = false;
			data.open_kline = 0;
		}
	}

	if (dia.base->_state == 2) {		//取到一根已完成K线
		if (open) {
			//若已经开仓，则只需要更新当日最高/低价以及最新的K线
			update_dayprice_and_seq(dia, data);
			get_vol_and_kline(id, dia, data);
		} else {		//若还未开仓，则还需要计算成交量最大的K线
			cons_forward_10kline(id, dia, data);
		}
	}
}

int max10vol1min::check_kline(dia_group& dia, ins_data& data) {
	if (dia.ext->close_item.price > dia.ext->open_item.price)
		return 1;		//收盘价大于开盘价，阳线1

	if (dia.ext->close_item.price < dia.ext->open_item.price)
		return -1;		//收盘价小于开盘价，阴线-1

	return 0;		//十字架，返回0，未知阴阳线
}

void max10vol1min::get_vol_and_kline(const std::string& id, dia_group& dia, ins_data& data) {
	data.get_opvol_and_kline = true;
	data.open_vol = dia.base->_volume_tick;
	data.open_kline = check_kline(dia, data);

	std::string kline_type = "非阴阳线";
	if (1 == data.open_kline)
		kline_type = "阳线";

	if (-1 == data.open_kline)
		kline_type = "阴线";
	print_thread_safe("[get_vol_and_kline date=%d seq=%d id=%s]获取到开仓点的那根已完成K线，成交量=%lld，K线类型=%d（%s）\n",
			dia.base->_date, dia.base->_seq, id.c_str(), data.open_vol, data.open_kline, kline_type.c_str());
}

void max10vol1min::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (seq_outdate(dia, data))
		return;

	if (!data.get_opvol_and_kline && dia.base->_state == 2)		//在开仓之后取到一根已完成的K线
		get_vol_and_kline(id, dia, data);

	if ((STS_OPEN_BUY == data.sts || STS_AMP_50_PER_BUY == data.sts) && depth->base.nPrice < data.mvol_it->max_price) {
		//最新价跌破该K线的最高点，止损
		print_thread_safe("[try_close_position date=%d seq=%d id=%s]在平（多）仓过程中，当前最新价（%lld）小于成交量最大的那根K线的最高点（%lld），"
				"止损平多仓！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice, data.mvol_it->max_price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		data.sts = STS_CLOSE;
	}

	if ((STS_OPEN_SELL == data.sts || STS_AMP_50_PER_SELL == data.sts) && depth->base.nPrice > data.mvol_it->min_price) {
		//最新价突破该K线的最低点，止损
		print_thread_safe("[try_close_position date=%d seq=%d id=%s]在平（空）仓过程中，当前最新价（%lld）大于成交量最大的那根K线的最低点（%lld），"
				"止损平空仓！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice, data.mvol_it->min_price);
		exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		data.sts = STS_CLOSE;
	}

	check_stop_profit(id, depth, dia, data);			//赢利达到当日波动幅度50%后，再回调70%止赢
	check_vol_and_kline(id, depth, dia, data);		//检查当前成交量与K线类型是否满足指定条件

	if (STS_CLOSE != data.sts && depth->base.nTime >= m_close_time) {
		print_thread_safe("[try_close_position date=%d time=%d id=%s]即将收盘，在收盘前强制平仓！\n",
				depth->base.nActionDay, depth->base.nTime, id.c_str());
		if (STS_OPEN_BUY == data.sts || STS_AMP_50_PER_BUY == data.sts)
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		else		//STS_OPEN_SELL == data.sts || STS_AMP_50_PER_SELL == data.sts
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
	}

	if (dia.base->_state == 2)
		update_dayprice_and_seq(dia, data);

	if (STS_CLOSE == data.sts) {
		print_thread_safe("[try_close_position date=%d seq=%d id=%s]本轮交易完成！\n", dia.base->_date, dia.base->_seq, id.c_str());
		data.sts = STS_INIT;
		data.far10k.clear();
	}
}

void max10vol1min::check_stop_profit(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (STS_CLOSE == data.sts)
		return;

	__int64 buy, sell, profit, amplitude = data.day_max-data.day_min;
	if (STS_OPEN_BUY == data.sts || STS_AMP_50_PER_BUY == data.sts) {
		buy = data.open_price;
		sell = depth->base.nPrice;
	}

	if (STS_OPEN_SELL == data.sts || STS_AMP_50_PER_SELL == data.sts) {
		buy = depth->base.nPrice;
		sell = data.open_price;
	}
	profit = sell-buy;

	if (profit >= amplitude*0.5) {
		if (STS_OPEN_BUY == data.sts) {
			data.sts = STS_AMP_50_PER_BUY;
			data.profit = profit;
			print_thread_safe("[check_stop_profit date=%d seq=%d id=%s]在平（多）仓过程中，当前最新价（%lld）与开仓点（%lld）"
					"之差（%lld）超过当日波动幅度（%lld）的50%！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice,
					data.open_price, profit, amplitude);
			return;
		}

		if (STS_OPEN_SELL == data.sts) {
			data.sts = STS_AMP_50_PER_SELL;
			data.profit = profit;
			print_thread_safe("[check_stop_profit date=%d seq=%d id=%s]在平（空）仓过程中，当前最新价（%lld）与开仓点（%lld）"
					"之差（%lld）超过当日波动幅度（%lld）的50%！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice,
					data.open_price, profit, amplitude);
			return;
		}
	}

	if (STS_AMP_50_PER_BUY != data.sts && STS_AMP_50_PER_SELL != data.sts)
		return;

	if (profit > data.profit) {
		print_thread_safe("[check_stop_profit date=%d seq=%d id=%s]在平仓过程中，最新价（%lld）与开仓点的价格（%lld）之差=%lld，大于之前的"
				"赢利值=%lld，更新！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice, data.open_price, profit, data.profit);
		data.profit = profit;
	} else if (profit <= data.profit*0.7) {
		//当前赢利回调70%，止赢
		if (STS_AMP_50_PER_BUY == data.sts) {
			print_thread_safe("[check_stop_profit date=%d seq=%d id=%s]在平（多）仓过程中，当前最新价（%lld）导致赢利又回调为原来（%lld）的70%，"
					"止盈平多仓！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice, data.profit);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		} else {		//STS_AMP_50_PER_SELL == data.sts
			print_thread_safe("[check_stop_profit date=%d seq=%d id=%s]在平（空）仓过程中，当前最新价（%lld）导致赢利又回调为原来（%lld）的70%，"
					"止盈平空仓！\n", dia.base->_date, dia.base->_seq, id.c_str(), depth->base.nPrice, data.profit);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		}
		data.sts = STS_CLOSE;
	}
}

void max10vol1min::check_vol_and_kline(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (STS_CLOSE == data.sts)
		return;		//之前已经平仓，此处不再检查

	if (0 == data.open_kline)		//开仓K线非阴阳线，不再比较
		return;

	if (dia.base->_state != 2)		//该K线是阴线还是阳线只有在完成时才确定
		return;

	if (dia.base->_volume_tick <= data.open_vol)
		return;

	//当前K线成交量大于开仓K线成交量
	bool reverse = false;
	int kline_type = check_kline(dia, data);
	if (1 == data.open_kline && -1 == kline_type)
		reverse = true;

	if (-1 == data.open_kline && 1 == kline_type)
		reverse = true;

	if (reverse) {
		//在成交量满足条件之后，这根K线呈阴（阳）线与开仓方向相反，平仓止赢（赢or损？）
		if (STS_OPEN_BUY == data.sts || STS_AMP_50_PER_BUY == data.sts) {
			print_thread_safe("[check_vol_and_kline date=%d seq=%d id=%s]在平（多）仓过程中，当前K线的成交量（%lld）大于"
					"开仓K线的成交量（%lld），且当前已完成K线的阴阳线（%d）与开仓（%d）方向相反，平仓止赢！\n", dia.base->_date,
					dia.base->_seq, id.c_str(), dia.base->_volume_tick, data.open_vol,kline_type, data.open_kline);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
		}

		if (STS_OPEN_SELL == data.sts || STS_AMP_50_PER_SELL == data.sts) {
			print_thread_safe("[check_vol_and_kline date=%d seq=%d id=%s]在平（空）仓过程中，当前K线的成交量（%lld）大于"
					"开仓K线的成交量（%lld），且当前已完成K线的阴阳线（%d）与开仓（%d）方向相反，平仓止赢！\n", dia.base->_date,
					dia.base->_seq, id.c_str(), dia.base->_volume_tick, data.open_vol, kline_type, data.open_kline);
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
		}
		data.sts = STS_CLOSE;
	}
}

void max10vol1min::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_kline_idx))
		return;

	for (auto& dia : group.dias[m_kline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void max10vol1min::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		cons_forward_10kline(id, dia, data);
		break;
	}
	case STS_10KLINE_READY: {
		try_open_position(id, depth, dia, data);
		break;
	}
	case STS_OPEN_BUY:
	case STS_AMP_50_PER_BUY:
	case STS_OPEN_SELL:
	case STS_AMP_50_PER_SELL: {
		try_close_position(id, depth, dia, data);
		break;
	}

	default:
		break;
	}
}
