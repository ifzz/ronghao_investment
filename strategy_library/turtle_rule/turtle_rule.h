#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_20KLINE_ALREADY,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
};

struct klineavg {
	unsigned int date;
	unsigned int seq;
	__int64 kavg50;
	__int64 kavg250;
	__int64 high_price;
	__int64 low_price;
};

struct ins_data {
	STG_STATUS sts;
	char kavg_loca;		//1：50均线在250均线之上，做多，-1：50均线在250均线之下，做空
	__int64 kline20_high, kline20_low;		//20根K线的最高/低价
	__int64 open_price;		//开仓点的价格
	wd_seq last_klseq;		//最新的已完成K线的序号
	std::list<klineavg> kline20, kline10;		//开仓时用到的20根K线，平仓时用到的10根K线
	__int64 pdc;		//前一根K线的收盘价 tr=max(h-l, h-pdc, pdc-l)
	__int64 pdn;		//前一根K线的N值 ，初值为tr N=(19×pdn+tr)/20
	__int64 kline10_high, kline10_low;
	TradeUUID uuid;

	ins_data()
	:sts(STS_INIT)
	,kavg_loca(0)
	,pdn(-1) {
		last_klseq.vir_seq = 0;
	}
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
	void con_open_20kline(dia_group& dia, ins_data& data, bool is_his);
	void check_kavg_loca(ins_data& data);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void con_close_10kline(dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia,
			OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline, m_50kavgstr, m_250kavgstr;
	int32_t m_15minkline, m_50kavg, m_250kavg;
	std::map<std::string, ins_data> m_ins_data;
};
