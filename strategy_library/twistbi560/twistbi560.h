#pragma once

#include <deque>
#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_TBI_OVER,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
	STS_CLOSE_BUY,
	STS_CLOSE_SELL,
};

struct bikavg {
	unsigned int seq;
	__int64 bi;
	__int64 kavg60;
};

struct ins_data {
	STG_STATUS sts;
	unsigned int bi_seq;		//只取最新的seq，来的是旧的简单丢弃
	__int64 last_ding, last_di, last_macd_diff;
	char kavg60_loca;		//60均线的位置，1表示60均线在顶的上面，-1表示在底的下面
	std::list<bikavg> bi_ding;
	std::list<bikavg> bi_di;
	TradeUUID uuid;

	ins_data()
	:sts(STS_INIT)
	,bi_seq(0)
	,last_ding(-1)
	,last_di(-1)
	,kavg60_loca(0) {}
};

class twistbi560 : public strategy_base {
public:
	twistbi560() {
		m_strategy_name = "twist560";
	}
	virtual ~twistbi560() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	bool check_ding_form(ins_data& data);
	bool check_di_form(ins_data& data);
	bool prev_twistbi_changed(dia_group& dia, ins_data& data);
	void twist_init(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void cons_ding_form(MarketAnalyseTagBase *twist, MarketAnalyseTagBase *kavg60, MarketAnalyseKline *ext, ins_data& data);
	void cons_di_form(MarketAnalyseTagBase *twist, MarketAnalyseTagBase *kavg60, MarketAnalyseKline *ext, ins_data& data);
	bool check_kavg60_location(ins_data& data);
	bool check_twistbi_exist(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void record_last_twistbi(dia_group& dia, ins_data& data);
	bool try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data, bool check_twistbi);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
			OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_5minkline_idx, m_twistbi_idx, m_60avg_idx;
	std::map<std::string, ins_data> m_ins_data;
};
