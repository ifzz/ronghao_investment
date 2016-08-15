#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_PRICE_READY,
	STS_OPEN_BUY,
	STS_OPEN_SELL,
	STS_GT_OB_SELL,
	STS_LT_OB_BUY,
};

struct ins_data {
	STG_STATUS sts;
	wd_seq last_kseq;
	__int64 ob_sell_price;		//观察卖出价
	__int64 rb_sell_price;		//反转卖出价
	__int64 rb_buy_price;		//反转买入价

	__int64 ob_buy_price;	//观察买入价
	__int64 bt_buy_price;		//突破买入价
	__int64 bt_sell_price;		//突破卖出价
	TradeUUID uuid;

	ins_data() {
		last_kseq.vir_seq = 0;
	}
};

class rollbbtall : public strategy_base {
public:
	rollbbtall() {
		m_strategy_name = "反转突破策略";
	}
	virtual ~rollbbtall() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	bool seq_outdate(dia_group& dia, ins_data& data);
	void cons_base_price(dia_group& dia, ins_data& data, bool open);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data);
	void check_price_tendency(MarketDepthData *depth, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group&dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_kline_idx;
	std::map<std::string, ins_data> m_ins_data;
};
