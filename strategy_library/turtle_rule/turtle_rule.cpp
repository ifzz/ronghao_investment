#include "turtle_rule.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<turtle_rule>();
}

void turtle_rule::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
}

void turtle_rule::init() {
	m_15minkline = m_type_map[m_kline].type_index;
	m_50kavg = m_type_map[m_kline].tags["50均线kavg"];
	m_250kavg = m_type_map[m_kline].tags["250均线kavg"];
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

void turtle_rule::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_15minkline))
		return;

	for (auto& dia : group.dias[m_15minkline])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

void turtle_rule::con_20kline(dia_group& dia, ins_data& data) {
	if (dia.base->_state != 2)
		return;		//构造前20根K线时只需要关注已完成的K线

	//已完成
	MarketAnalyseTagBase *kavg50 = dia.tags[m_50kavg];
	MarketAnalyseTagBase *kavg250 = dia.tags[m_250kavg];
	if (!kavg50 || !kavg250)
		return;		//需要50均线和250均线

	if (kavg250->_value == kavg50->_value) {
		data.kline20.clear();		//过滤掉所有250和50均线相等的情形
		return;
	}

	data.kline20.push_back({kavg50->_value, kavg250->_value,
		dia.ext->max_item.price, dia.ext->min_item.price});
	if (20 == data.kline20.size())
		check_kavg_loca(data);
}

void turtle_rule::check_kavg_loca(ins_data& data) {
	auto& last_klineavg = data.kline20.back();
	//50均线在250均线之上时设置为1，否则为-1
	char last_loca = (last_klineavg.kavg50 > last_klineavg.kavg250) ? 1 : -1;
	if (1 == last_loca) {
		auto rit = data.kline20.rbegin();
		for (++rit; rit != data.kline20.rend(); ++rit)
			if (rit->kavg50 < rit->kavg250)
				break;

		if (rit == data.kline20.rend()) {		//20根K线的50均线都在250均线之上，尝试做多
			data.sts = STS_20KLINE_ALREADY;
			data.kavg_loca = 1;

			for (auto& kline : data.kline20)		//记录20根K线的最高价
				if (kline.high_price > data.kline20_high)
					data.kline20_high = kline.high_price;
		} else {
			std::advance(rit, 2);		//删除旧的K线，继续构造20根K线，尝试做空
			data.kline20.erase(data.kline20.begin(), rit.base());
		}
	} else {		//-1 == last_loca
		auto rit = data.kline20.rbegin();
		for (++rit; rit != data.kline20.rend(); ++rit)
			if (rit->kavg50 > rit->kavg250)
				break;

		if (rit == data.kline20.rend()) {		//20根K线的50均线都在250均线之下
			data.sts = STS_20KLINE_ALREADY;
			data.kavg_loca = -1;

			for (auto& kline : data.kline20)		//记录20根K线的最低价
				if (kline.low_price < data.kline20_low)
					data.kline20_low = kline.low_price;
		} else {
			std::advance(rit, 2);
			data.kline20.erase(data.kline20.begin(), rit.base());
		}
	}
}

void turtle_rule::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	if (1 != data.kavg_loca || -1 != data.kavg_loca)
		return;

	if (1 == data.kavg_loca) {		//尝试做多

	} else {		//-1 == data.kavg_loca		尝试做空

	}
}

void turtle_rule::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {

}

void turtle_rule::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {
		con_20kline(dia, data);
		break;
	}
	case STS_20KLINE_ALREADY: {
		try_open_position(id, depth, dia, data);
		break;
	}
	case STS_OPEN_BUY:
	case STS_OPEN_SELL: {
		break;
	}
	}
}
