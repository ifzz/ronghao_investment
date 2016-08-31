#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_10KLINE_READY,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
};

struct ins_data {
	STG_STATUS sts;
	wd_seq last_kseq;
	TradeUUID uuid;

	__int64 A;
	std::list<__int64> day10slist;

	ins_data() {
		last_kseq.vir_seq = 0;
	}
};

class days10HCL : public strategy_base {
public:
	days10HCL() {
		m_strategy_name = "取10根日线开平仓";
	}
	virtual ~days10HCL() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	bool seq_outdate(dia_group& dia, ins_data& data);
	void cons_10days_hcl(const std::string& id, dia_group& dia, ins_data& data);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_kline_idx;
	uint32_t m_close_time;
	std::map<std::string, ins_data> m_ins_data;
};
