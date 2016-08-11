#ifndef __Stock_Msg_H
#define __Stock_Msg_H

#ifndef __int64
#define __int64 long long
#endif

enum  Stock_Msg
{
	Stock_Msg_Nothing ,
	Stock_Msg_InstrumentList, //合约列表
	Stock_Msg_DepthMarket, 	//深度行情
	Stock_Msg_DiagramInfo, //数据指标列表(通过行情分析后的数据，如K线，均线、缠论等)
	Stock_Msg_DiagramList, //图表数据的合约列表
	Stock_Msg_DiagramData,	//分析处理后生成的数据
	Stock_Msg_DiagramTag,	//分析处理后生成的标签
	Stock_Msg_SubscribeAll,	//订阅所有合约
	Stock_Msg_UnSubscribeAll,	//订阅所有合约
	Stock_Msg_SubscribeById,//订阅指定合约
	Stock_Msg_UnSubscribeById,//订阅指定合约
	Stock_Msg_SubscribeSpeed,
	Stock_Msg_UnSubscribeSpeed,
	Stock_Msg_SubscribeLiveness,
	Stock_Msg_UnSubscribeLiveness,
	Stock_Msg_SpeedTop20,
	Stock_Msg_LivenessTop20,
	Stock_Msg_DiagramGroup, //多条图表数据混合(可减少消息的传输数量，提高效率)
	Stock_Msg_DepthHistoryReq,
	Stock_Msg_DiagramHistoryReq,
	Stock_Msg_DepthHistoryData,
	Stock_Msg_DiagramHistoryData,
	Stock_Msg_DiagramHistoryTag,

	TRADE_MSG_INPUT_ORDER,

	Stock_Msg_DiagramCacheData, //衔接实时数据和历史数据的中间数据
	Stock_Msg_DiagramCacheTag,	//衔接实时数据和历史数据的中间tag

	Stock_Msg_Max,

};

enum Trade_Msg
{
	Trade_Msg_SnatchSet = 1000, //开盘前抢单设置
	Trade_Msg_TrendSet,				//趋势开仓设置
	Trade_Msg_DiffSet,				//买卖价差设置
	Stock_Msg_Trade_Ok, 			//成交通知
	Trade_Msg_SnatchCancel,	 	//取消抢单任务
	Trade_Msg_SnatchOver, 		//抢单完成
	Trade_Msg_ShowTime,

	Trade_Msg_StrategeOpen,			//程序化策率开仓
	Trade_Msg_StrategeClose,		//程序化策率平仓
	Trade_Msg_StrategeForceClose,	//程序化强制平仓，用于风控

	Trade_Msg_Max
};


#endif
