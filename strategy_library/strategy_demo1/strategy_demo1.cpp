#include "strategy_demo1.h"

 std::shared_ptr<strategy_base> create_strategy() {
	 return std::make_shared<strategy_demo1>();
 }

 void strategy_demo1::init() {
	 m_1minkline_index = m_type_map["1分钟kline"].type_index;
 }

 void strategy_demo1::execute_trade(const std::string& ins_id, dia_group& dia, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
	 datetime market;
	 market.date = dia.ext->close_item.date;
	 market.time = dia.ext->close_item.time;

	 order_instruction oi;
	 if (FLAG_CLOSE == flag)
		 oi.uuid = m_ins_data[ins_id].uuid;

	 oi.market = market;
	 oi.direction = direction;
	 oi.flag = flag;
	 oi.price = dia.ext->close_item.price;

	 oi.vol_cnt = 1;
	 oi.level = 1;

	 oi.type_index = m_1minkline_index;
	 oi.dia_seq = dia.base->_seq;
	 request_trade(ins_id, oi);
	 if (FLAG_OPEN == flag)
		 m_ins_data[ins_id].uuid = oi.uuid;
 }

 /*
  * 这个策略关注1分钟k线
  * 若当前这根K线的
  * 最新价（收盘价）高于前一根K线的最高价，那么产生开仓行为（做多）同时平前一个空仓
  * 最新价低于前一根K线的最低价，产生开仓行为（做空）同时平前一个多仓
  */

 void strategy_demo1::execute(depth_dia_group& group) {
	 if (group.dias.end() == group.dias.find(m_1minkline_index))
		 return;			//只关注1分钟k线

	 for (auto& dia : group.dias[m_1minkline_index])
		 go(group.ins_id, group.depth.get(), dia);
 }

 void strategy_demo1::go(const std::string& ins_id, MarketDepthData *depth, dia_group& dia) {
	 if (m_ins_data.end() == m_ins_data.find(ins_id))
		 m_ins_data[ins_id] = ins_data();
	 auto& data = m_ins_data[ins_id];

	 //当前状态非STATUS_INIT时将有可能产生开仓行为，而开仓总是需要上一根K线的最高/低价
	 if (data.m_sts != STATUS_INIT) {
		 if (dia.base->_seq > data.m_last_k_line._seq) {
			 data.m_max_price = data.m_last_k_line.m_max_price;
			 data.m_min_price = data.m_last_k_line.m_min_price;
			 print_thread_safe("[demo1]当前处于非初始化状态，有可能产生开仓行为，记录上一根K线的最高/低价："
					 "id=%s, max_price=%d, min_price=%d\n", ins_id.c_str(), data.m_max_price, data.m_min_price);
		 } else if (dia.base->_seq < data.m_last_k_line._seq) {
			 return;		//直接忽略这个包，说明更新了之前data下的一个tag
		 }
	 }

	 switch (data.m_sts) {
	 case STATUS_INIT: {		//确定第一根K线
		 if (2 != dia.base->_state)
			 return;

		 data.m_sts = STATUS_READY;		//表示已经处于可以开仓的状态
		 print_thread_safe("[demo1]确定第一根K线，当前处于可开仓的状态：id=%s, seq=%d\n", ins_id.c_str(), dia.base->_seq);
		 break;
	 }
	 case STATUS_READY: {		//此处产生第一次开仓行为
		 if ((dia.ext->close_item.price > data.m_max_price) || (dia.ext->close_item.price < data.m_min_price)) {
			 if (dia.ext->close_item.price > data.m_max_price) {		//做多
				 print_thread_safe("[demo1]当前K线的最新价高于上一根K线的最高价，开多仓：id=%s, 最新价=%d，上一最高价=%d\n",
						 ins_id.c_str(), dia.ext->close_item.price, data.m_max_price);
				 execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_BUY);
				 data.m_sts = STATUS_OPEN_BUY;
			 } else if (dia.ext->close_item.price < data.m_min_price) {		//做空
				 print_thread_safe("[demo1]当前K线的最新价低于上一根K线的最高价，开空仓：id=%s, 最新价=%d，上一最低价=%d\n",
						 ins_id.c_str(), dia.ext->close_item.price, data.m_min_price);
				 execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_SELL);
				 data.m_sts = STATUS_OPEN_SELL;
			 }
		 }
		 break;
	 }
	 case STATUS_OPEN_BUY: {		//买多
		 if (dia.ext->close_item.price < data.m_min_price) {
			 //当前最新价小于上一根K线的最低价，卖平并做空
			 execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_SELL);
			 execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_SELL);
			 print_thread_safe("[demo1]当前K线的最新价低于上一根K线的最低价，卖平并做空：id=%s, 最新价=%d，上一最低价=%d\n",
					 ins_id.c_str(), dia.ext->close_item.price, data.m_min_price);
			 data.m_sts = STATUS_OPEN_SELL;
		 }
		 break;
	 }
	 case STATUS_OPEN_SELL: {
		 if (dia.ext->close_item.price > data.m_max_price) {
			 //当前最新价大于上一根K线的最高价，买平并买多
			 execute_trade(ins_id, dia, FLAG_CLOSE, DIRECTION_BUY);
			 execute_trade(ins_id, dia, FLAG_OPEN, DIRECTION_BUY);
			 print_thread_safe("[demo1]当前K线的最新价高于上一根K线的最高价，买平并做多：id=%s, 最新价=%s，上一最高价=%d\n",
					 ins_id.c_str(), dia.ext->close_item.price, data.m_max_price);
			 data.m_sts = STATUS_OPEN_BUY;
		 }
		 break;
	 }
	 default:
		 break;
	 }

	 //总是记录上一根K线的序号、最高价和最低价
	 data.m_last_k_line._seq = dia.base->_seq;
	 data.m_last_k_line.m_max_price = dia.ext->max_item.price;
	 data.m_last_k_line.m_min_price = dia.ext->min_item.price;
 }
