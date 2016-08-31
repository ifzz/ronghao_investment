#include "turtle_rule.h"
#include <algorithm>
#include <assert.h>

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<turtle_rule>();
}

void turtle_rule::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
	m_50kavgstr = conf["test_50kavg"];
	m_250kavgstr = conf["test_250kavg"];
}

void turtle_rule::init() {
	m_15minkline = m_type_map[m_kline].type_index;
	m_50kavg = m_type_map[m_kline].tags[m_50kavgstr];
	m_250kavg = m_type_map[m_kline].tags[m_250kavgstr];

	for (auto& ins : m_ins_info) {
		auto& data = m_ins_data[ins.first];
		for_each_his_dia(ins.first, 15, "分钟", "kline", -1, [&](dia_group& dia, void *args)->void {
			con_open_20kline(dia, data, true);
		}, nullptr);
	}
}

void turtle_rule::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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
	oi.type_index = m_15minkline;
	request_trade(id, oi);
	if (FLAG_OPEN == flag)
		m_ins_data[id].uuid = oi.uuid;
}

void turtle_rule::con_open_20kline(dia_group& dia, ins_data& data, bool is_his) {
	if (dia.base->_state != 2)
		return;

	if (dia.mode == -1)
		return;

	MarketAnalyseTagBase *kavg50 = dia.tags[m_50kavg];
	MarketAnalyseTagBase *kavg250 = dia.tags[m_250kavg];
	if (!kavg50 || !kavg250)
		return;		//需要50均线和250均线

	if (kavg250->_value == kavg50->_value) {
		data.kline20.clear();		//过滤掉所有250和50均线相等的情形
		return;
	}

	wd_seq uni_seq;
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_klseq.vir_seq)		//如果是旧的K线，则尝试更新以前的值
		return;

	if (data.kline20.size() == 20)
		data.kline20.pop_front();

	//新的K线
	data.kline20.push_back({dia.base->_date, dia.base->_seq, kavg50->_value, kavg250->_value,
		dia.ext->max_item.price, dia.ext->min_item.price});
	data.pdc = dia.ext->close_item.price;
	data.last_klseq.date = dia.base->_date;
	data.last_klseq.seq = dia.base->_seq;
	print_thread_safe("[con_open_20kline]得到一根已完成的K线！date=%d seq=%d 最高价=%d 最低价=%d\n",
			dia.base->_date, dia.base->_seq, dia.ext->max_item.price, dia.ext->min_item.price);
	if (20 == data.kline20.size() && !is_his)
		check_kavg_loca(data);
}

void turtle_rule::check_kavg_loca(ins_data& data) {
	auto rit = data.kline20.rbegin();
	char last_loca = (rit->kavg50 > rit->kavg250) ? 1 : -1;		//50均线在250均线之上时设置为1，否则为-1
	if (1 == last_loca) {
		for (++rit; rit != data.kline20.rend(); ++rit)
			if (rit->kavg50 < rit->kavg250)
				break;

		if (rit == data.kline20.rend()) {		//20根K线的50均线都在250均线之上，尝试做多
			data.sts = STS_20KLINE_ALREADY;
			data.kavg_loca = 1;
			//记录20根K线的最高价
			data.kline20_high = std::max_element(data.kline20.begin(), data.kline20.end(),
					[](const klineavg& i, const klineavg& j)->bool { return i.high_price < j.high_price; })->high_price;
			print_thread_safe("[check_kavg_loca]20根K线的50均线都在250均线之上，尝试做多！最高价为%d\n",
					data.kline20_high);
		} else {
			int org_size = data.kline20.size();
			data.kline20.erase(data.kline20.begin(), rit.base());
			print_thread_safe("[check_kavg_loca]20根K线的50均线和250均线发生交叉，已删除过时K线！last_loca=%d，"
					"删除个数=%d\n", last_loca, org_size-data.kline20.size());
		}
	} else {		//-1 == last_loca
		for (++rit; rit != data.kline20.rend(); ++rit)
			if (rit->kavg50 > rit->kavg250)
				break;

		if (rit == data.kline20.rend()) {		//20根K线的50均线都在250均线之下
			data.sts = STS_20KLINE_ALREADY;
			data.kavg_loca = -1;
			//记录20根K线的最低价
			data.kline20_low = std::min_element(data.kline20.begin(), data.kline20.end(),
					[](const klineavg& i, const klineavg& j)->bool { return i.low_price < j.low_price; })->low_price;
			print_thread_safe("[check_kavg_loca]20根K线的50均线都在250均线之下，尝试做空！最低价为%d\n",
					data.kline20_low);
		} else {
			int org_size = data.kline20.size();
			data.kline20.erase(data.kline20.begin(), rit.base());
			print_thread_safe("[check_kavg_loca]20根K线的50均线和250均线发生交叉，已删除过时K线！last_loca=%d，"
					"删除个数=%d\n", last_loca, org_size-data.kline20.size());
		}
	}
}

void turtle_rule::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	wd_seq uni_seq;
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_klseq.vir_seq)
		return;		//来的是一个旧的K线，突破时用的是最新价

	if (1 == data.kavg_loca) {		//尝试做多
		if (depth->base.nPrice > data.kline20_high) {
			data.sts = STS_OPEN_BUY;
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
			print_thread_safe("[try_open_position]最新价突破20根K线的最高点，且50均线在250均线之上，开多仓！date=%d "
					"seq=%d 最新价（开仓点价格）=%d 最高点=%d\n", dia.base->_date, dia.base->_seq,
					depth->base.nPrice, data.kline20_high);
		}
	} else {		//-1 == data.kavg_loca		尝试做空
		if (depth->base.nPrice < data.kline20_low) {
			data.sts = STS_OPEN_SELL;
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
			print_thread_safe("[try_open_position]最新价突破20根K线的最低点，且50均线在250均线之下，开空仓！date=%d "
					"seq=%d 最新价（开仓点价格）=%d 最高点=%d\n", dia.base->_date, dia.base->_seq,
					depth->base.nPrice, data.kline20_low);
		}
	}

	if (data.sts == STS_OPEN_BUY || data.sts == STS_OPEN_SELL) {
		//已经开仓
		data.open_price = depth->base.nPrice;		//开仓点的价格
		if (dia.base->_state == 2) {		//如果在完整状态的K线处开仓，那么记录pdc
			data.pdc = dia.ext->close_item.price;
			data.last_klseq.date = dia.base->_date;
			data.last_klseq.seq = dia.base->_seq;
		}
		return;
	}

	if (dia.base->_state == 2) {
		//在尝试开仓时遇到一根已完成的K线，重新构造新的20根K线
		print_thread_safe("[try_open_position]在尝试开仓过程中取到一根已完成的K线，打破之前形态，重新构造！\n");
		data.sts = STS_INIT;
		con_open_20kline(dia, data, false);
	}
}

void turtle_rule::con_close_10kline(dia_group& dia, ins_data& data) {
	if (dia.base->_state != 2)
		return;		//平仓时构造的10根K线都是已完成的

	if (data.kline10.size() == 10)
		data.kline10.pop_front();

	data.kline10.push_back({0, 0, 0, 0});		//只需要这10根K线的最高价和最低价
	data.kline10.back().high_price = dia.ext->max_item.price;
	data.kline10.back().low_price = dia.ext->min_item.price;
	print_thread_safe("[con_close_10kline]在平仓过程中构造10根K线，当前取到一根已完成的K线 date=%d seq=%d "
			"最高价=%d 最低价=%d\n", dia.base->_date, dia.base->_seq, dia.ext->max_item.price, dia.ext->min_item.price);

	if (data.kline10.size() == 10) {
		//10根K线的最高价
		data.kline10_high = std::max_element(data.kline10.begin(), data.kline10.end(),
				[](const klineavg& i, const klineavg& j)->bool { return i.high_price < j.high_price; })->high_price;

		data.kline10_low = std::min_element(data.kline10.begin(), data.kline10.end(),
				[](const klineavg& i, const klineavg& j)->bool { return i.low_price < j.low_price; })->low_price;
		print_thread_safe("[con_close_10kline]平仓过程中取满连续的10根K线 最高点=%d 最低点=%d\n",
				data.kline10_high, data.kline10_low);
	}
}

void turtle_rule::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	wd_seq uni_seq;
	uni_seq.date = dia.base->_date;
	uni_seq.seq = dia.base->_seq;
	if (uni_seq.vir_seq <= data.last_klseq.vir_seq)
		return;

	__int64 h = dia.ext->max_item.price, l = dia.ext->min_item.price;
	__int64 TR = std::max(std::max(h-l, h-data.pdc), data.pdc-l);		//TR（实际范围）=max(H-L, H-PDC, PDC-L)

	__int64 N = TR;
	if (data.pdn != -1)
		N = (19*data.pdn+TR)/20;
	print_thread_safe("[try_close_position]尝试平仓：最高价=%d 最低价=%d PDC(上一根K线的收盘价)=%d "
			"TR(实际范围)=%d PDN=%d N=%d\n", h, l, data.pdc, TR, data.pdn, N);

	bool close = false;
	if (1 == data.kavg_loca) {		//平多仓
		if (depth->base.nPrice < data.open_price-2*N) {		//最新价突破开仓点价格-2N，止损
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			print_thread_safe("[try_close_position 平多仓]最新价突破开仓点价格-2N，止损！date=%d seq=%d, 最新价=%d "
					"开仓点价格=%d", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.open_price);
			close = true;
		}

		if (data.kline10.size() == 10 && depth->base.nPrice < data.kline10_low) {
			//或者价格突破近10根K线最低点时退出，止盈或止损
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			print_thread_safe("[try_close_position 平多仓]最新价突破近10根K线最低点，退出！date=%d seq=%d 最新价=%d "
					"近10根K线最低点=%d", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.kline10_low);
			close = true;
		}
		if (dia.base->_state == 2)		//只需要已完成的10根K线
			con_close_10kline(dia, data);
	} else {		//-1 == data.kavg_loca		//平空仓
		if (depth->base.nPrice > data.open_price+2*N) {		//最新价突破开仓点价格+2N，止损
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			print_thread_safe("[try_close_position 平空仓]最新价突破开仓点价格+2N，止损！date=%d seq=%d, 最新价=%d "
					"开仓点价格=%d", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.open_price);
			close = true;
		}

		if (data.kline10.size() == 10 && depth->base.nPrice > data.kline10_high) {
			//或者价格突破近10根K线的最高点时退出，止盈或止损
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			print_thread_safe("[try_close_position 平空仓]最新价突破近10根K线最高点，退出！date=%d seq=%d 最新价=%d "
					"近10根K线最高点=%d", dia.base->_date, dia.base->_seq, depth->base.nPrice, data.kline10_high);
			close = true;
		}
		if (dia.base->_state == 2)
			con_close_10kline(dia, data);
	}

	if (close) {
		data.sts = STS_INIT;
		data.pdn = -1;
		data.kline10.clear();
		data.kline20.clear();
		print_thread_safe("[try_close_position]平仓完成 开始新一轮交易！\n");
		return;
	}

	if (dia.base->_state == 2) {
		//当前K线已完成，记录PDN——前一根K线完成时的N值
		data.pdn = N;
		data.pdc = dia.ext->close_item.price;
		data.last_klseq.date = dia.base->_date;
		data.last_klseq.seq = dia.base->_seq;
		print_thread_safe("[try_close_position]平仓过程中取到一根已完成的K线，记录pdn=%d, date=%d, seq=%d\n",
				data.pdn, dia.base->_date, dia.base->_seq);
	}
}

void turtle_rule::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_15minkline))
		return;

	for (auto& dia : group.dias[m_15minkline])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void turtle_rule::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		con_open_20kline(dia, data, false);
		break;
	}
	case STS_20KLINE_ALREADY: {
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
