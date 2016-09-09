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
	unsigned int date;
	unsigned int seq;
	__int64 bi;
};

struct ins_data {
	STG_STATUS sts;
	wd_seq bi_seq;		//只取最新的seq，来的是旧的简单丢弃
	wd_seq kline_seq;		//记录最新的K线
	__int64 last_ding, last_di, last_macd_diff;
	std::list<bikavg> bi_ding;
	std::list<bikavg> bi_di;
	TradeUUID uuid;

	ins_data()
	:sts(STS_INIT)
	,last_ding(-1)
	,last_di(-1)
	,last_macd_diff(0) {
		bi_seq.vir_seq = 0;
		kline_seq.vir_seq = 0;
	}
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
	bool prev_twistbi_changed(dia_group& dia, ins_data& data);
	void twist_init(const std::string& id, dia_group& dia, ins_data& data);
	void cons_ding_form(MarketAnalyseTagBase *twist, MarketAnalyseDataBase *base, MarketAnalyseKline *ext, ins_data& data);
	void cons_di_form(MarketAnalyseTagBase *twist, MarketAnalyseDataBase *base, MarketAnalyseKline *ext, ins_data& data);
	bool check_twistbi_exist(const std::string& id, dia_group& dia, ins_data& data);
	void record_last_twistbi(dia_group& dia, ins_data& data);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
			OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_5minkline_idx, m_twistbi_idx;
	std::map<std::string, ins_data> m_ins_data;
};
