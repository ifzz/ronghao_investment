#pragma once

#include "strategy_base.h"

typedef enum {
	STATUS_INIT = 0,
	STATUS_IN_OPEN_BUY,
	STATUS_IN_CLOSE_BUY,
	STATUS_IN_OPEN_SELL,
	STATUS_IN_CLOSE_SELL,
} K_AVERAGE_STATUS;

class five_minute_kline_1 : public strategy_base {
public:
	five_minute_kline_1();
	virtual ~five_minute_kline_1() {}

	virtual void init();
	virtual void execute(depth_dia_group& group);

private:
	K_AVERAGE_STATUS m_status;
	unsigned int m_seq;
	bool m_five_greater_than_ten;

	int32_t m_12Skline_index, m_5kavg_index, m_10kavg_index, m_60kavg_index;
};
