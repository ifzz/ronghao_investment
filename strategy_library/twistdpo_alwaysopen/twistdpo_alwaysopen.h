#pragma once

#include "strategy_base.h"

enum STG_STATUS {
	STS_INIT = 0,
};

struct ins_data {
	STG_STATUS sts;

	ins_data()
	:sts(STS_INIT) {}
};

class twistdpo_alwaysopen : public strategy_base {
public:
	twistdpo_alwaysopen() {
		m_strategy_name = "twistdpo_alwaysopen";
	}
	virtual ~twistdpo_alwaysopen() {}

	virtual void read_conf(std::map<std::string, const char*>& conf);
	virtual void init();
	virtual void execute(depth_dia_group& group);

private:

private:
	std::string m_kline;
	int32_t m_kline_idx;
	std::map<std::string, ins_data> m_ins_data;
};
