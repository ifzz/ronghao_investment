#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_PRICE_READY,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
	STS_FL_S_SETUP,
	STS_GU_B_SETUP,

	STS_CLOSE_GU_S_SET,
	STS_CLOSE_FL_B_SET,
};

struct ins_data {
	STG_STATUS sts;
	wd_seq last_kseq;
	TradeUUID uuid;

	// H-昨日最高价  C-昨日收盘价  L-昨日最低价
	__int64 B_break;		// = H+2×（P-L）
	__int64 S_setup;			// = P+H-L
	__int64 S_enter;			// = 2×P-L

	__int64 B_enter;			// = 2×P-H
	__int64 B_setup;		// = P-H+L
	__int64 S_break;		// = L-2×（H-P）

	ins_data() {
		last_kseq.vir_seq = 0;
	}
};

class rollbbtall_ver2 : public strategy_base {
public:
	rollbbtall_ver2() {
		m_strategy_name = "反转突破策略（版本2）";
	}
	virtual ~rollbbtall_ver2() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	bool seq_outdate(dia_group& dia, ins_data& data);
	void cons_base_price(dia_group& dia, ins_data& data, bool open);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void check_price_tendency(MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void until_price_btop_again(MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_kline_idx;
	std::map<std::string, ins_data> m_ins_data;
};
