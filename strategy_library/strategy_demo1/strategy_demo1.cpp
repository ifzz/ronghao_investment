#include "strategy_demo1.h"

 std::shared_ptr<strategy_base> create_strategy() {
	 return std::make_shared<strategy_demo1>();
 }

 void strategy_demo1::init() {
	 strategy_base::init();
	 m_1minkline_index = m_type_map["1分钟kline"].type_index;
 }

 void strategy_demo1::execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
	 MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	 datetime market;
	 market.date = ext->close_item.date;
	 market.time = ext->close_item.time;

	 price_model model;
	 model.type = ORDER_LIMIT;
	 model.price = ext->close_item.price;

	 order_instruction oi;
	 size_t size = m_strategy_name.size() > 15 ? 15 : m_strategy_name.size();
	 memcpy(oi.stg_id, m_strategy_name.c_str(), size);
	 oi.stg_id[size] = '\0';
	 oi.type_index = m_1minkline_index;
	 oi.seq = group.diagrams[m_1minkline_index].base->_seq;
	 oi.level = 1;
	 oi.market = market;
	 oi.model = model;
	 oi.flag = flag;
	 oi.direction = direction;
	 request_trade(group.ins_id, oi);
 }

 /*
  * 这个策略关注1分钟k线
  * 若当前这根K线的最新价（收盘价）高于前一根K线的最高价，那么产生开仓行为（做多）
  * 反之若其最新价低于前一根K线的最低价，那么做空
  */

 void strategy_demo1::execute(depth_dia_group& group) {
	 if (group.diagrams.end() == group.diagrams.find(m_1minkline_index))
		 return;		//只关注1分钟k线

	 if (m_ins_data.end() == m_ins_data.find(group.ins_id))
		 m_ins_data[group.ins_id] = ins_data();

	 MarketAnalyseDataBase *base = group.diagrams[m_1minkline_index].base;
	 MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	 auto& data = m_ins_data[group.ins_id];

	 //当前状态非STATUS_INIT时将有可能产生开仓行为，而开仓总是需要上一根K线的最高/低价
	 if (data.m_sts != STATUS_INIT) {
		 if (base->_seq > data.m_last_k_line._seq) {
			 data.m_max_price = data.m_last_k_line.m_max_price;
			 data.m_min_price = data.m_last_k_line.m_min_price;
		 } else if (base->_seq < data.m_last_k_line._seq) {
			 return;		//直接忽略这个包，说明来的是一个tag数据
		 }
	 }

	 switch (data.m_sts) {
	 case STATUS_INIT: {		//确定第一根K线
		 if (2 != base->_state)
			 return;

		 data.m_sts = STATUS_READY;		//表示已经处于可以开仓的状态
		 break;
	 }
	 case STATUS_READY: {		//此处产生第一次开仓行为
		 if ((ext->close_item.price > data.m_max_price) || (ext->close_item.price < data.m_min_price)) {
			 if (ext->close_item.price > data.m_max_price) {		//做多
				 execute_trade(group, FLAG_OPEN, DIRECTION_BUY);
				 data.m_sts = STATUS_OPEN_BUY;
			 } else if (ext->close_item.price < data.m_min_price) {		//做空
				 execute_trade(group, FLAG_OPEN, DIRECTION_SELL);
				 data.m_sts = STATUS_OPEN_SELL;
			 }
		 }
		 break;
	 }
	 case STATUS_OPEN_BUY: {		//买多
		 if (ext->close_item.price < data.m_min_price) {
			 //当前最新价小于上一根K线的最低价，卖平并做空
			 execute_trade(group, FLAG_CLOSE, DIRECTION_SELL);
			 execute_trade(group, FLAG_OPEN, DIRECTION_SELL);
			 data.m_sts = STATUS_OPEN_SELL;
		 }
		 break;
	 }
	 case STATUS_OPEN_SELL: {
		 if (ext->close_item.price > data.m_max_price) {
			 //当前最新价大于上一根K线的最高价，买平并买多
			 execute_trade(group, FLAG_CLOSE, DIRECTION_BUY);
			 execute_trade(group, FLAG_OPEN, DIRECTION_BUY);
			 data.m_sts = STATUS_OPEN_BUY;
		 }
		 break;
	 }
	 default:
		 break;
	 }

	 //总是记录上一根K线的序号、最高价和最低价
	 data.m_last_k_line._seq = base->_seq;
	 data.m_last_k_line.m_max_price = ext->max_item.price;
	 data.m_last_k_line.m_min_price = ext->min_item.price;
 }
