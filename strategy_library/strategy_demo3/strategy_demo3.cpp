#include "strategy_demo3.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<strategy_demo3>();
}

void strategy_demo3::init() {
	strategy_base::init();
	m_12skline_index = m_type_map["12秒kline"].type_index;
}

void strategy_demo3::execute_trade(depth_dia_group& group, TRADE_DIRECTION direction, int price) {
	datetime market;
	market.date = group.depth->base.nTradeDay;
	market.time = group.depth->base.nTime;

	price_model model;
	model.type = ORDER_LIMIT;
	model.price = price;

 	order_instruction oi;
 	oi.type_index = m_12skline_index;
 	oi.seq = group.diagrams[m_12skline_index].base->_seq;
 	oi.flag = FLAG_OPEN;
 	oi.direction = direction;
 	oi.level = 1;
 	oi.market = market;
 	oi.model = model;
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

	auto& depth = group.depth;
	if (m_deal_price != -1) {
		int64_t tick = m_ins_info[group.ins_id].price_tick;
		if (abs(depth->base.bid_ask.Ask-depth->base.bid_ask.Bid) >= 1*tick) {
			//当前买价和卖价之差5个tick，进一步观察成交价
			if ((depth->base.nPrice > m_deal_price) || (depth->base.nPrice < m_deal_price)) {
				print_thread_safe("[strategy_demo3]当前深度行情的买价：%d，卖价：%d，tick：%d, "
						"且本次成交价为：%d，上一次成交价为：%d\n", depth->base.bid_ask.Bid,
						depth->base.bid_ask.Ask, tick, depth->base.nPrice, m_deal_price);

				if (depth->base.nPrice > m_deal_price)		//当前成交价高于前一个深度行情成交价，做多
					execute_trade(group, DIRECTION_BUY, depth->base.bid_ask.Bid+tick);
				else if (depth->base.nPrice < m_deal_price)		//当前成交价低于前一个深度行情成交价，做空
					execute_trade(group, DIRECTION_SELL, depth->base.bid_ask.Ask-tick);
			 }
		 }
	 }

	m_deal_price = depth->base.nPrice;
 }
