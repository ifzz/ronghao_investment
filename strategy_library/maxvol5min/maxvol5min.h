#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_VOL_DETERM,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
};

struct ins_data {
	STG_STATUS sts;
	wd_seq last_kseq;
	TradeUUID uuid;

	__int64 volume_tick;		//成交量
	__int64 max_price;			//最高点
	__int64 min_price;			//最低点
	__int64 stop_price;			//止损价
	__int64 last_price;			//平仓时的上一价格

	ins_data()
	:sts(STS_INIT)
	,volume_tick(-1) {
		last_kseq.vir_seq = 0;
	}
};

class maxvol5min : public strategy_base {
public:
	maxvol5min() {
		m_strategy_name = "5分钟K线——当日成交量最大的K线作为基准线";
	}
	virtual ~maxvol5min() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	bool seq_outdate(dia_group& dia, ins_data& data);
	bool determ_maxvol(const std::string& id, dia_group& dia, ins_data& data, bool sts_trans);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void check_10tick_amplitude(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data, int64_t tick);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_kline_idx;
	uint32_t m_close_time;
	std::map<std::string, ins_data> m_ins_data;
};
