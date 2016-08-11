#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_20KLINE_ALREADY,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
};

struct klineavg {
	__int64 kavg50;
	__int64 kavg250;
	__int64 high_price;
	__int64 low_price;
};

struct ins_data {
	STG_STATUS sts;
	char kavg_loca;
	__int64 kline20_high, kline20_low;
	std::list<klineavg> kline20;
	TradeUUID uuid;

	ins_data()
	:sts(STS_INIT)
	,kavg_loca(0)
	,kline20_high(INT64_MIN)
	,kline20_low(INT64_MAX) {}
};

class turtle_rule : public strategy_base {
public:
	turtle_rule() {
		m_strategy_name = "海龟法则";
	}
	virtual ~turtle_rule() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	void con_20kline(dia_group& dia, ins_data& data);
	void check_kavg_loca(ins_data& data);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
			OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_15minkline, m_50kavg, m_250kavg;
	std::map<std::string, ins_data> m_ins_data;
};
