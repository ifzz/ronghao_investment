#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
	STS_10KLINE_READY,

	STS_OPEN_BUY,
	STS_OPEN_SELL,
	STS_AMP_50_PER_BUY,
	STS_AMP_50_PER_SELL,

	STS_CLOSE,
};

struct far10kline {
	__int64 max_price;		//最高价
	__int64 min_price;		//最低价
	__int64 vol_tick;			//成交量
};

struct ins_data {
	STG_STATUS sts;
	wd_seq last_kseq;
	TradeUUID uuid;

	__int64 day_max;		//当日最高价
	__int64 day_min;		//当日最低价
	__int64 open_price;	//开仓价
	__int64 open_vol;		//开仓成交量

	bool get_opvol_and_kline;
	char open_kline;		//开仓K线，阳线：1，阴线：-1，非阴阳线：0

	std::list<far10kline> far10k;		//前10根K线
	std::list<far10kline>::iterator mvol_it;		//指向far10k中成交量最大的元素的迭代器

	__int64 profit;

	ins_data()
	:sts(STS_INIT)
	,day_max(INT64_MIN)
	,day_min(INT64_MAX)
	,get_opvol_and_kline(false)
	,open_kline(0) {
		last_kseq.date = get_current_datetime().date;
	}
};

class max10vol1min : public strategy_base {
public:
	max10vol1min() {
		m_strategy_name = "1分钟K线——取前10根中成交量最大的那根K线作为基准";
	}
	virtual ~max10vol1min() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

public:
	bool seq_outdate(dia_group& dia, ins_data& data);
	void update_dayprice_and_seq(dia_group& dia, ins_data& data);
	void cons_forward_10kline(const std::string& id, dia_group& dia, ins_data& data);
	void try_open_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	int check_kline(dia_group& dia, ins_data& data);
	void get_vol_and_kline(const std::string& id, dia_group& dia, ins_data& data);
	void try_close_position(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void check_stop_profit(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);
	void check_vol_and_kline(const std::string& id, MarketDepthData *depth, dia_group& dia, ins_data& data);

	void sts_trans(const std::string& id, MarketDepthData *depth, dia_group& dia);
	void exec_trade(const std::string& id, MarketDepthData *depth, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION dir);

private:
	std::string m_kline;
	int32_t m_kline_idx;
	uint32_t m_close_time;
	std::map<std::string, ins_data> m_ins_data;
};
