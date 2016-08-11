#include "twistbi560.h"
#include <assert.h>
#include <algorithm>

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<twistbi560>();
}

void twistbi560::read_conf(std::map<std::string, const char*>& conf) {
	m_kline = conf["test_tgt"];
}

void twistbi560::init() {
	m_5minkline_idx = m_type_map[m_kline].type_index;
	m_twistbi_idx = m_type_map[m_kline].tags["0笔twist_bi"];
	m_60avg_idx = m_type_map[m_kline].tags["60均线kavg"];
}

void twistbi560::exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
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
	oi.type_index = m_5minkline_idx;
	request_trade(id, oi);
	if (FLAG_OPEN == flag)
		m_ins_data[id].uuid = oi.uuid;
}

bool twistbi560::check_ding_form(ins_data& data) {
	auto next = data.bi_ding.rbegin();
	auto it = next++;
	while (next != data.bi_ding.rend()) {
		if (it->bi >= next->bi)		//出现当前的ding大于等于前一个ding
			break;
		++it;
		++next;
	}
	if (next == data.bi_ding.rend())
		return false;

	std::advance(next, 2);		//next指向的ding也要删
	data.bi_ding.erase(data.bi_ding.begin(), next.base());
	data.sts = STS_INIT;
	return true;
}

bool twistbi560::check_di_form(ins_data& data) {
	auto next = data.bi_di.rbegin();
	auto it = next++;
	while (next != data.bi_di.rend()) {
		if (it->bi <= next->bi)		//出现当前的di小于等于前一个di
			break;
		++it;
		++next;
	}
	if (next == data.bi_di.rend())
		return false;

	std::advance(next, 2);
	data.bi_di.erase(data.bi_di.begin(), next.base());
	data.sts = STS_INIT;
	return true;
}

//若之前形态发生变化则返回true，否则返回false
bool twistbi560::prev_twistbi_changed(dia_group& dia, ins_data& data) {
	bool changed = false;
	MarketAnalyseTagBase *twist = dia.tags[m_twistbi_idx];
	if (twist->_type > 0) {		//ding 0-删除  1-新增  2-修改
		auto it = data.bi_ding.begin();
		for (; it != data.bi_ding.end(); ++it)
			if (it->date == twist->_date && it->seq == twist->_seq)
				break;		//date & seq 均相同
		if (it == data.bi_ding.end())		//没有找到
			return false;

		if (0 == dia.tag_mode[m_twistbi_idx]) {		//删除
			data.bi_ding.erase(it);
			data.sts = STS_INIT;
			print_thread_safe("[prev_twistbi_changed]前面的twistbi  ding发生删除操作 seq=%d\n", twist->_seq);
			return true;
		} else if (2 == dia.tag_mode[m_twistbi_idx]) {			//修改
			it->bi = twist->_value;
			changed = check_ding_form(data);
			if (changed)
				print_thread_safe("[prev_twistbi_changed]前面的twistbi ding发生修改操作 value=%d seq=%d ding_size=%d\n",
						twist->_value, twist->_seq, data.bi_ding.size());
		}
	} else {		//di
		auto it = data.bi_di.begin();
		for (; it != data.bi_di.end(); ++it)
			if (it->date == twist->_date && it->seq == twist->_seq)
				break;		//date & seq 均相同
		if (it == data.bi_di.end())
			return false;

		if (0 == dia.tag_mode[m_twistbi_idx]) {
			data.bi_di.erase(it);
			data.sts = STS_INIT;
			print_thread_safe("[prev_twistbi_changed]前面的twistbi  di发生删除操作 seq=%d\n", twist->_seq);
			return true;
		} else if (2 == dia.tag_mode[m_twistbi_idx]) {
			it->bi = twist->_value;
			changed = check_di_form(data);
			if (changed)
				print_thread_safe("[prev_twistbi_changed]前面的twistbi di发生修改操作 value=%d seq=%d di_size=%d\n",
						twist->_value, twist->_seq, data.bi_di.size());
		}
	}
	return changed;
}

void twistbi560::twist_init(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	MarketAnalyseTagBase *twist = dia.tags[m_twistbi_idx];
	MarketAnalyseTagBase *kavg60 = dia.tags[m_60avg_idx];
	if (!twist || !kavg60 || 0 == twist->_type)
		return;		//未知类型的bi

	wd_seq cur_seq;
	cur_seq.date = twist->_date;
	cur_seq.seq = twist->_seq;

	if (cur_seq.vir_seq <= data.bi_seq.vir_seq) {		//来了一个旧的bi
		prev_twistbi_changed(dia, data);
		return;
	}

	if (cur_seq.vir_seq > data.kline_seq.vir_seq) {		//记录最新的K线
		data.kline_seq.date = dia.base->_date;
		data.kline_seq.seq = dia.base->_seq;
	}

	if (twist->_type > 0)		//ding
		cons_ding_form(twist, kavg60, dia.ext, data);
	else		//twist->_type < 0		//di
		cons_di_form(twist, kavg60, dia.ext, data);
	data.bi_seq.date = twist->_date;		//最新的bi
	data.bi_seq.seq = twist->_seq;

	if (3 == data.bi_di.size() && 3 == data.bi_ding.size()) {
		print_thread_safe("[twist_init]已取满所有的ding和di，尝试开仓！\n");
//		if (check_kavg60_location(data)) {
//			print_thread_safe("[twist_init] 60均线的位置满足既定要求，尝试开仓！\n");
			if (!try_open_position(id, depth, dia, data, false))		//尝试开仓
				data.sts = STS_TBI_OVER;
//		}
	}
}

void twistbi560::cons_ding_form(MarketAnalyseTagBase *twist, MarketAnalyseTagBase *kavg60,
		MarketAnalyseKline *ext, ins_data& data) {
	if (!data.bi_ding.empty() && data.bi_ding.back().bi <= twist->_value)
		data.bi_ding.clear();		//ding的形态被破坏，将之前的ding清空

	if (data.bi_ding.size() == 3)
		data.bi_ding.pop_front();

	data.bi_ding.push_back({twist->_date, twist->_seq, twist->_value, kavg60->_value});
	data.last_ding = twist->_value;
	data.last_macd_diff = ext->diff;
	print_thread_safe("[cons_ding_form] 取到的是ding（%d），value=%d, seq=%d, date=%d\n",
			data.bi_ding.size(), twist->_value, twist->_seq, twist->_date);
}

void twistbi560::cons_di_form(MarketAnalyseTagBase *twist, MarketAnalyseTagBase *kavg60,
		MarketAnalyseKline *ext, ins_data& data) {
	if (!data.bi_di.empty() && data.bi_di.back().bi >= twist->_value)
		data.bi_di.clear();		//di的形态被破坏，将之前的di清空

	if (data.bi_di.size() == 3)
		data.bi_di.pop_front();

	data.bi_di.push_back({twist->_date, twist->_seq, twist->_value, kavg60->_value});
	data.last_di = twist->_value;
	data.last_macd_diff = ext->diff;
	print_thread_safe("[cons_di_form] 取到的是di（%d），value=%d, seq=%d, date=%d\n",
			data.bi_di.size(), twist->_value, twist->_seq, twist->_date);
}

bool twistbi560::check_kavg60_location(ins_data& data) {
	int gt_zero_cnt = 0, lt_zero_cnt = 0;
	for (auto& ding : data.bi_ding) {
		if (ding.kavg60 > ding.bi)
			gt_zero_cnt++;
		else if (ding.kavg60 < ding.bi)
			lt_zero_cnt++;
	}
	for (auto& di : data.bi_di) {
		if (di.kavg60 > di.bi)
			gt_zero_cnt++;
		else if (di.kavg60 < di.bi)
			lt_zero_cnt++;
	}
	print_thread_safe("[check_kavg60_location]已取满所有的ding和di，60均线大于所有ding和di的个数为%d，"
			"60均线小于所有ding和di的个数为%d\n", gt_zero_cnt, lt_zero_cnt);

	if (!gt_zero_cnt || !lt_zero_cnt) {
		if (!gt_zero_cnt)
			data.kavg60_loca = -1;
		if (!lt_zero_cnt)
			data.kavg60_loca = 1;
		return true;
	}
	return false;		//60均线发生穿越，出错
}

bool twistbi560::check_twistbi_exist(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	MarketAnalyseTagBase *twist = dia.tags[m_twistbi_idx];
	if (!twist || 0 == twist->_type)
		return false;

	wd_seq cur_seq;
	cur_seq.date = twist->_date;
	cur_seq.seq = twist->_seq;
	if (cur_seq.vir_seq <= data.bi_seq.vir_seq)		//来的是一个旧的twistbi
		return prev_twistbi_changed(dia, data);

	//来的是一个新的twistbi
	if (twist->_type > 0)		//检测到是ding
		data.bi_ding.pop_front();
	else if (twist->_type < 0)		//检测到是di
		data.bi_di.pop_front();
	print_thread_safe("[check_twistbi_exist]在尝试开仓过程中检测到有新的twistbi value=%d, seq=%d, date=%d\n",
			twist->_value, twist->_seq, twist->_date);
	data.sts = STS_INIT;
	twist_init(id, depth, dia, data);
	return true;
}

bool twistbi560::try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia,
		ins_data& data, bool check_twistbi) {
	//尝试开仓是需要时刻关注最新的bi有没有破坏当前的形态，若是则重新选取
	if (check_twistbi && check_twistbi_exist(id, depth, dia, data))
		return false;

	wd_seq cur_seq;
	cur_seq.date = dia.base->_date;
	cur_seq.seq = dia.base->_seq;
	if (cur_seq.vir_seq < data.kline_seq.vir_seq) {
		return false;
	} else {
		data.kline_seq.date = dia.base->_date;
		data.kline_seq.seq = dia.base->_seq;
	}

//	if (data.kavg60_loca == -1) {		//ding和di都在60均线之上
		if (data.last_macd_diff > 0 && dia.ext->close_item.price > data.bi_ding.back().bi) {
			data.sts = STS_OPEN_BUY;		//开多仓
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_BUY);
			print_thread_safe("[try_open_position date=%d seq=%d]当前K线的最新价为%d，最近的ding的值为%d，macd中diff字段"
					"的值为%d（>0），开多仓！\n", dia.base->_date, dia.base->_seq, dia.ext->close_item.price, data.bi_ding.back().bi, dia.ext->diff);
			return true;
		}
//	} else {		//ding和di都在60均线之下
		if (data.last_macd_diff < 0 && dia.ext->close_item.price < data.bi_di.back().bi) {
			data.sts = STS_OPEN_SELL;		//开空仓
			exec_trade(id, depth, dia, FLAG_OPEN, DIRECTION_SELL);
			print_thread_safe("[try_open_position date=%d seq=%d]当前K线的最新价为%d，最近的di的值为%d，macd中diff字段"
					"的值为%d（<0），开空仓！\n", dia.base->_date, dia.base->_seq, dia.ext->close_item.price, data.bi_di.back().bi, dia.ext->diff);
			return true;
		}
//	}
	return false;
}

void twistbi560::record_last_twistbi(dia_group& dia, ins_data& data) {
	MarketAnalyseTagBase *twist = dia.tags[m_twistbi_idx];
	if (!twist || 0 == twist->_type)
		return;		//无效的bi

	//在开仓之后不管之前的形态是否发生破坏，都只需要记录最新的ding和最新的di，观察是否需要平仓
	wd_seq cur_seq;
	cur_seq.date = twist->_date;
	cur_seq.seq = twist->_seq;
	if (cur_seq.vir_seq < data.bi_seq.vir_seq)
		return;

	if (twist->_type > 0)		//ding
		data.last_ding = twist->_value;
	else if (twist->_type < 0)		//di
		data.last_di = twist->_value;
	data.bi_seq.date = twist->_date;
	data.bi_seq.seq = twist->_seq;
	print_thread_safe("[record_last_twistbi]检测到最新的ding（%d）和di（%d）的值 && 当前K线的date=%d, seq=%d\n",
			data.last_ding, data.last_di, twist->_date, twist->_seq);
}

void twistbi560::try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data) {
	record_last_twistbi(dia, data);

	wd_seq cur_seq;
	cur_seq.date = dia.base->_date;
	cur_seq.seq = dia.base->_seq;
	if (cur_seq.vir_seq < data.kline_seq.vir_seq) {
		return;
	} else {
		data.kline_seq.date = dia.base->_date;
		data.kline_seq.seq = dia.base->_seq;
	}

	bool close = false;
	if (STS_OPEN_BUY == data.sts) {
		//平多仓
		if (dia.ext->close_item.price < data.last_di) {
			data.sts = STS_CLOSE_SELL;
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_SELL);
			print_thread_safe("[try_close_position]当前K线的最新价为%d，最新的di的值为%d，该di的序列号为%d，平多仓！\n",
					dia.ext->close_item.price, data.last_di, dia.base->_seq);
			close = true;
		}
	} else if (STS_OPEN_SELL == data.sts) {
		//平空仓
		if (dia.ext->close_item.price > data.last_ding) {
			data.sts = STS_CLOSE_BUY;
			exec_trade(id, depth, dia, FLAG_CLOSE, DIRECTION_BUY);
			print_thread_safe("[try_close_position]当前K线的最新价为%d，最新的ding的值为%d，该ding的序列号为%d，平空仓！\n",
					dia.ext->close_item.price, data.last_ding, dia.base->_seq);
			close = true;
		}
	}

	if (close) {
		data.bi_di.clear();
		data.bi_ding.clear();
		data.sts = STS_INIT;
	}
}

void twistbi560::execute(depth_dia_group& group) {
	if (group.dias.end() == group.dias.find(m_5minkline_idx))
		return;		//关注5分钟K线

	for (auto& dia : group.dias[m_5minkline_idx])
		sts_trans(group.ins_id, group.depth.get(), dia);
}

/**
 * 当前这个策略只关注5分钟K线
 * 选取twist_bi中连续的三个顶和三个底，并且这三个顶和底都在60均线之上，要求三个顶的值连续递减且三个底的值连续
 * 递增，并且三个顶的值都大于三个底的值，满足以上条件之后，若进一步有当前K线的最新价大于三个顶中最新的那个顶
 * 的值并且macd中的diff字段大于0，那么就开多仓，反之三个顶和三个底都在60均线之下，并且如果最新价小于最新的
 * 那个底的值并且macd中的diff小于0，那么开空仓
 * ->对于多仓，若当前K线的最新价小于最新的底的值那么平多仓
 * ->对于空仓，若当前K线的最新价大于最新的顶的值那么平空仓
 */

void twistbi560::sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia) {
	if (m_ins_data.end() == m_ins_data.find(id))
		m_ins_data[id] = ins_data();
	auto& data = m_ins_data[id];

	switch (data.sts) {
	case STS_INIT: {		//初始
		twist_init(id, depth, dia, data);
		break;
	}
	case STS_TBI_OVER: {
		try_open_position(id, depth, dia, data, true);
		break;
	}
	case STS_OPEN_BUY:
	case STS_OPEN_SELL: {
		try_close_position(id, depth, dia, data);
		break;
	}
	default:
		break;
	}
}
