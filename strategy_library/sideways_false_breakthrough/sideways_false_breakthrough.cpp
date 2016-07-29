#include "sideways_false_breakthrough.h"

std::shared_ptr<strategy_base> create_strategy() {
	return std::make_shared<sideways_false_breakthrough>();
 }

 sideways_false_breakthrough::sideways_false_breakthrough()
 :m_20_kline_is_ready(false)
 ,m_max_highest_price(0)
 ,m_min_lowest_price((unsigned int)UINTMAX_MAX)
 ,m_middle_price(0)
 ,m_avg_volume(0)
 ,m_offset_open(false)
 ,m_open_highest(0)
 ,m_open_lowest(0)
 ,m_close_level(0)
 ,m_node_sts(STATUS_INIT)
 ,m_1minkline_index(-1) {
 }

 sideways_false_breakthrough::~sideways_false_breakthrough() {
	 m_group_queue.clear();
 }

 void sideways_false_breakthrough::init() {
	 strategy_base::init();
	 m_1minkline_index = m_type_map["1分钟kline"].type_index;
 }

/*
 * name：横盘假突破
 * 这个策略只关注1分钟K线：
 * 首先取20根K线，在这20根K线中查找最高价中的最大值p1以及最低价中的最小值p2，取这两个值的中值m=(p1+p2)/2，
 * 设置当前这个策略关注的浮动范围为[m-5, m+5]，这20根K线的持仓量和成交量的均值分别设为c1和c2
 *
 * 对于这20根K线的后续K线来说，若该K线的收盘价先是>m+5，并在此K线的完成过程中这一收盘价又回落至(m-25, m+5)之间时，
 * 同时持仓量和成交量小于c1和c2的3倍，那么产生开仓（对于持仓量来说，如果当前开仓点在15s，那么持仓量要按照c的60/15=4倍
 * 与c1进行比较），此时主要做空仓。但是一旦收盘价 <= m-25，则重新选取20根K线
 *
 * 空仓的平仓点主要发生在以下三个时点上（h指的是同一根K线在完成过程中的最高收盘价）：
 * 		=>①后续K线的收盘价一直在[m, h]之间浮动，而突然有一根K线的收盘价>h，则平仓
 * 		=>②后续K线的收盘价一直在[m-25, m]之间浮动，突然有一根K线的收盘价>m，平仓
 * 		=>③后续有一根K线的收盘价<m-25，直接平仓
 *
 * 	多仓行为与空仓行为相反。
 *
 */

 void sideways_false_breakthrough::check_whether_satisfy_status(int64_t tick) {
	 //已取得20根K线，判断这20根K线是否满足状态
	 m_middle_price = (m_max_highest_price+m_min_lowest_price)/2;
	 for (auto& group : m_group_queue) {
		 MarketAnalyseKline *in = group.diagrams[m_1minkline_index].ext;
		 if (in->max_item.price > m_middle_price+5*tick || in->min_item.price < m_middle_price-5*tick) {
			 //当前这个包的「最高价 > m+5*tick」 或者「最低价 < m-5*tick」，那么说明这20根K线不符合要求
			 m_group_queue.pop_front();
			 m_20_kline_is_ready = false;
			 return;
		 }
	 }

	 int64_t total_volume = 0;
	 for (auto& group : m_group_queue)
		 total_volume += group.diagrams[m_1minkline_index].base->_volume_tick;
	 m_avg_volume = total_volume*1.0f/m_group_queue.size();

	 MarketAnalyseDataBase *first_base = m_group_queue.front().diagrams[m_1minkline_index].base;
	 MarketAnalyseKline *first_ext = m_group_queue.front().diagrams[m_1minkline_index].ext;
	 MarketAnalyseDataBase *last_base = m_group_queue.back().diagrams[m_1minkline_index].base;
	 MarketAnalyseKline *last_ext = m_group_queue.back().diagrams[m_1minkline_index].ext;
	 print_thread_safe("「20根K线已经选取成功！」第一根K线的时间点是%d:%d，序号是%d，"
			 "最后一根K线的时间点是%d:%d，序号是%d，平均成交量为%f，最高价中的最大值为%d, "
			 "最低价中的最小值为%d，从而选取的均值是%d\n",
			 first_ext->open_item.date, first_ext->open_item.time,  first_base->_seq,
			 last_ext->close_item.date, last_ext->close_item.time, last_base->_seq,
			 m_avg_volume, m_max_highest_price, m_min_lowest_price, m_middle_price);
	 m_20_kline_is_ready = true;
 }

 void sideways_false_breakthrough::construct_20_kline(depth_dia_group& group, int64_t tick) {
	 m_group_queue.push_back(group);

	 MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	 if (ext->max_item.price > m_max_highest_price)
		 m_max_highest_price = ext->max_item.price;		//记录最高价
	 else if (ext->min_item.price < m_min_lowest_price)
		 m_min_lowest_price = ext->min_item.price;			//记录最低价

	 if (m_group_queue.size() == 20)		//已取满20个K线节点，检测这些节点是否满足状态
		 check_whether_satisfy_status(tick);
 }

 void sideways_false_breakthrough::reselect_20_kline(depth_dia_group& group, int64_t tick) {
	 m_20_kline_is_ready = false;
	 m_node_sts = STATUS_INIT;
	 m_group_queue.pop_front();

	 if (2 == group.diagrams[m_1minkline_index].base->_state)		//当前节点是一个已完成节点
		 construct_20_kline(group, tick);
 }

 void sideways_false_breakthrough::execute_trade(depth_dia_group& group, OFFSET_FLAG flag, TRADE_DIRECTION direction) {
	 MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	 datetime market;
	 market.date = ext->close_item.date;
	 market.time = ext->close_item.time;

	 price_model model;
	 model.type = ORDER_LIMIT;
	 model.price = ext->close_item.price;

	 order_instruction oi;
	 oi.type_index = m_1minkline_index;
	 oi.seq = group.diagrams[m_1minkline_index].base->_seq;
	 oi.level = 1;
	 oi.market = market;
	 oi.model = model;
	 oi.flag = flag;
	 oi.direction = direction;
	 request_trade(group.ins_id, oi);
 }

 void sideways_false_breakthrough::execute(depth_dia_group& group) {
	 if (group.diagrams.end() == group.diagrams.find(m_1minkline_index))
		 return;

	 MarketAnalyseDataBase *base = group.diagrams[m_1minkline_index].base;
	 MarketAnalyseKline *ext = group.diagrams[m_1minkline_index].ext;
	 int64_t tick = m_ins_info[group.ins_id].price_tick;

	 if (!m_offset_open) {
		 //未开仓，检测是否需要开仓
		 if (!m_20_kline_is_ready) {		//20根K线还没确定下来
			 //此时只需关注已完成的K线
			 if (2 == base->_state)
				 construct_20_kline(group, tick);

			 return;
		 }

		 //已确定好20根K线，进行开仓，开仓行为关注的是同一个K线节点内收盘价的变化
		 //注意若此时已取定满足条件的20根K线，那么接下来的包必定属于一个新的K线节点
		 if (STATUS_INIT == m_node_sts) {

			 if (ext->close_item.price > m_middle_price+5*tick) {
				 m_node_sts = STATUS_BT_M_PLUS_FIVE;			//收盘价突破m+5
				 m_open_highest = ext->close_item.price;
				 print_thread_safe("「sideways_false_breakthrough」当前这根K线从初始化状态突破m+5=%d...\n",
						 m_middle_price+5*tick);
			 } else if (ext->close_item.price < m_middle_price-5*tick) {
				 m_node_sts = STATUS_BT_M_REDUCE_FIVE;		//收盘价突破m-5
				 m_open_lowest = ext->close_item.price;
				 print_thread_safe("「sideways_false_breakthrough」当前这根K线从初始化状态突破m-5=%d...\n",
						 m_middle_price-5*tick);
			 }

		 } else if (STATUS_BT_M_PLUS_FIVE == m_node_sts) {		//之前已突破m+5
			 if (ext->max_item.price > m_open_highest) {
				 print_thread_safe("「sideways_false_breakthrough」当前这根K线最高价的最大值从%d突破到%d...\n", m_open_highest, ext->max_item.price);
				 m_open_highest = ext->close_item.price;
				 //在m+5和m_open_highest(h)之间波动时不做任何操作
			 }

			 if ( (ext->close_item.price < m_middle_price+5*tick) && (ext->close_item.price > m_middle_price-25*tick) ) {
				 //这一收盘价又回落到(m-25, m+5)之间，那么再检测(持仓量)成交量是否满足条件

				 if (base->_volume_tick < 3*m_avg_volume) {		//成交量小于均值的3倍，发出开仓行为（做空）
					 m_offset_open = true;
					 print_thread_safe("「sideways_false_breakthrough」当前这根K线的收盘价又回落到m+5=%d以内，"
							 "在(m-25, m+5)区间内，并且其成交量小于均值的3倍，发出卖空行为...\n", m_middle_price+5*tick);
					 execute_trade(group, FLAG_OPEN, DIRECTION_SELL);

					 if (ext->close_item.price > m_middle_price)
						 m_close_level = 1;		//处于(m, h)之间，当收盘价高于h时触发平仓操作
					 else
						 m_close_level = 2;		//处于(m-25, m)之间，收盘价高于m时触发平仓操作
				 } else {		//成交量大于等于均值的3倍，此时直接重新选取20根K线，因为成交量随时间单调递增，上述条件总是无法满足
					 print_thread_safe("「sideways_false_breakthrough」成交量大于等于均值的3倍，重新选取20根K线...\n");
					 reselect_20_kline(group, tick);
					 return;
				 }

			 } else if (ext->close_item.price <= m_middle_price-25*tick) {
				 //直接重新选取20个K线节点
				 print_thread_safe("「sideways_false_breakthrough」收盘价低于m-25=%d，选取的横盘范围不稳定，"
						 "重新选取新的20根K线\n", m_middle_price-25*tick);
				 reselect_20_kline(group, tick);
				 return;
			 }
		 } else {		//STATUS_BT_M_REDUCE_FIVE == m_node_sts，之前已突破m-5
			 if (ext->min_item.price < m_open_lowest) {
				 print_thread_safe("「sideways_false_breakthrough」当前这根K线最低价的最小值从%d降到%d...\n", m_open_lowest, ext->min_item.price);
				 m_open_lowest = ext->close_item.price;
				 //在m-5和m_open_lowest(l)之间波动时不做任何操作
			 }

			 if (ext->close_item.price > m_middle_price-5*tick && ext->close_item.price < m_middle_price+25*tick) {
				 //这一收盘价又上升至(m-5, m+25)之间，那么再检测成交量是否满足条件

				 if (base->_volume_tick < 3*m_avg_volume) {		//成交量小于均值3倍，发出开仓行为（做多）
					 m_offset_open = true;
					 print_thread_safe("「sideways_false_breakthrough」当前这根K线的收盘价又上升到m-5=%d以上，"
							 "在(m-5, m+25)区间内，并且其成交量小于均值的3倍，发出买多行为...\n", m_middle_price-5*tick);
					 execute_trade(group, FLAG_OPEN, DIRECTION_BUY);

					 if (ext->close_item.price < m_middle_price)
						 m_close_level = 1;		//处于(l, m)之间，当收盘价低于l时触发平仓操作
					 else
						 m_close_level = 2;		//处于(m, m+25)之间，收盘价低于m时触发平仓操作
				 } else {		//成交量大于等于均值的3倍，此时直接重新选取20根K线，因为成交量随时间单调递增，上述条件总是无法满足
					 print_thread_safe("「sideways_false_breakthrough」成交量大于等于均值的3倍，重新选取20根K线...\n");
					 reselect_20_kline(group, tick);
					 return;
				 }

			 } else if (ext->close_item.price >= m_middle_price+25*tick) {
				 print_thread_safe("「sideways_false_breakthrough」收盘价高于m+25=%d，选取的横盘范围不稳定，"
						 "重新选取新的20根K线\n", m_middle_price+25*tick);
				 reselect_20_kline(group, tick);
				 return;
			 }
		 }

		 if (2 == base->_state && !m_offset_open) {			//在这个节点完成过程中中没有发生开仓行为
			 print_thread_safe("「sideways_false_breakthrough」当前K线在完成过程中没有发生开仓行为，"
					 "重新选取20根K线");
			 reselect_20_kline(group, tick);
		 }

		 return;

	 } else {
		 //已开仓，检测是否需要平仓

		 if (STATUS_BT_M_PLUS_FIVE == m_node_sts) {
			 //此时开的是空仓

			 if (ext->close_item.price < m_middle_price-25*tick) {
				 //直接平仓，止盈
				 m_offset_open = false;
			 } else if (ext->close_item.price > m_open_highest) {
				 //直接平仓，止损
				 m_offset_open = false;
			 } else if (ext->close_item.price > m_middle_price) {
				 //之前处于(m-25, m)之间，本次突破m，在(m, h)之间，平仓
				 if (2 == m_close_level)
					 m_offset_open = false;
			 } else {
				 //在(m-25, m)之间
				 m_close_level = 2;
			 }

			 if (!m_offset_open)		//需要做平仓操作
				 execute_trade(group, FLAG_CLOSE, DIRECTION_BUY);
		 } else if (STATUS_BT_M_REDUCE_FIVE == m_node_sts) {
			 //此时开的是多仓
			 if (ext->close_item.price > m_middle_price+25*tick) {
				 //高于m+25，直接平仓，止盈
				 m_offset_open = false;
			 } else if (ext->close_item.price < m_open_lowest) {
				 //低于l，直接平仓，止损
				 m_offset_open = false;
			 } else if (ext->close_item.price < m_middle_price) {
				 //之前处于(m, m+25)之间，本次突破m，在(l, m)之间，平仓
				 if (2 == m_close_level)
					 m_offset_open = false;
			 } else {
				 //在(m, m+25)之间
				 m_close_level = 2;
			 }

			 if (!m_offset_open)
				 execute_trade(group, FLAG_CLOSE, DIRECTION_SELL);
		 }

		 if (!m_offset_open) {
			 m_20_kline_is_ready = false;
			 m_node_sts = STATUS_INIT;
			 m_group_queue.clear();
		 }
	 }
 }
