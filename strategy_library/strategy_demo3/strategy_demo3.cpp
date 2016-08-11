#include "strategy_demo3.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<strategy_demo3>();
}

void strategy_demo3::init() {
	m_12skline_index = m_type_map["12秒kline"].type_index;
}

void strategy_demo3::execute_trade(depth_dia_group& group, TRADE_DIRECTION direction, int price) {
	datetime market;
	market.date = group.depth->base.nTradeDay;
	market.time = group.depth->base.nTime;

 	order_instruction oi;
 	oi.market = market;
 	oi.direction = direction;
 	oi.flag = FLAG_OPEN;
 	oi.price = price;

 	oi.vol_cnt = 1;
 	oi.level = 1;

 	oi.type_index = m_12skline_index;
 	oi.dia_seq = group.dias[m_12skline_index].back().base->_seq;
 	request_trade(group.ins_id, oi);
}

 /*
  * 这个策略只关注深度行情数据，且执行过程中只存在开仓行为
  * 当一旦检测到当前这个深度行情的买卖价之差>=5个tick，那么就有可能触发开仓行为：
  * 	->若当前深度行情的成交价高于前一个深度行情的成交价，那么产生做多行为，且其开仓价比当前的买价高一个tick
  * 	->反之若低于前一个深度行情的成交价，产生做空行为，其开仓价比当前的卖价低一个tick
  */

void strategy_demo3::execute(depth_dia_group& group) {
	if (!group.depth)		//只关注tick级行情
		return;

	auto& ins = group.ins_id;
	auto& depth = group.depth;
	if (m_ins_deal.end() != m_ins_deal.find(ins)) {
		int64_t tick = m_ins_info[group.ins_id].price_tick;
		if (abs(depth->base.bid_ask.Ask-depth->base.bid_ask.Bid) >= 5*tick) {
			//当前买价和卖价之差5个tick，进一步观察成交价
			if ((depth->base.nPrice > m_ins_deal[ins]) || (depth->base.nPrice < m_ins_deal[ins])) {
				print_thread_safe("\n[debug]买价：%d，卖价：%d，tick：%d, "
						"本次成交价：%d，上一次成交价：%d\n", depth->base.bid_ask.Bid,
						depth->base.bid_ask.Ask, tick, depth->base.nPrice, m_ins_deal[ins]);

				if (depth->base.nPrice > m_ins_deal[ins])		//当前成交价高于前一个深度行情成交价，做多
					execute_trade(group, DIRECTION_BUY, depth->base.bid_ask.Bid+tick);
				else if (depth->base.nPrice < m_ins_deal[ins])		//当前成交价低于前一个深度行情成交价，做空
					execute_trade(group, DIRECTION_SELL, depth->base.bid_ask.Ask-tick);
			 }
		 }
	 }
	m_ins_deal[ins] = depth->base.nPrice;
 }
