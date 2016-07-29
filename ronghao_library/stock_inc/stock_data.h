#ifndef __Stock_Data_H
#define __Stock_Data_H

#ifndef __int64
#define __int64 long long
#endif

#pragma pack(1)

typedef struct PriceInfo
{
	__int64 			value;
	unsigned int 	volume;
}PriceInfo;

typedef struct MarketBidAsk
{
    __int64  Bid;//10档买,原始价格*10000
    __int64  Ask;//10档卖,原始价格*10000,
    unsigned int  BidVolume;
    unsigned int  AskVolume;
}MarketBidAsk;

//价格必须64bit数据才能保证x10000后不会越界450920000
typedef struct MarketDepthBase
{
	unsigned int  nTradeDay;//交易日期,YYYYMMDD 8位数字
	unsigned int  nActionDay; //业务日期YYYYMMDD 8位数字
	unsigned int  nTime; //'行情时间点 时分秒毫秒 如 093001500,表示09:30:01:500
	unsigned int	tick_volume; //相对成交量,方便后续使用
	__int64		   	nPrice;//'价格，单位1/10000',即原始价格*10000，避免后续浮点计算
	__int64		   	iVolume;  //成交总量
	__int64		   	iTurnover;//成交总金额，原始价格*10000，避免后续浮点计算
	MarketBidAsk  bid_ask;
	__int64 			AveragePrice; //平均价
	unsigned int  OpenInterest;		//当前持仓量
	char          nothing[4] ;//保留以后使用(16字节对齐，也方便查看文件)
}MarketDepthBase;

typedef struct MarketDepthExt //每天的数据中只需要存一份即可
{
	__int64		   PreClose; //昨收盘价格
	__int64		   OpenPrice; //今开盘价
	__int64		   HighestPrice; //最高价
	__int64		   LowestPrice; //最低价
	__int64		   UpperLimitPrice;	//涨停板价
	__int64		   LowerLimitPrice; //跌停板价
	__int64  	   PreOpenInterest; //昨持仓量
	__int64			 PreSettlementPrice;	//昨结算价
	__int64			 SettlementPrice;			//今天结算价
	__int64			 AveragePrice;			//今日平均价格

}MarketDepthExt;

typedef struct MarketDepthData
{
	MarketDepthBase 	base;
	MarketDepthExt	 	ext; //这个如果文件存放，每天只存一次，在 n*base的后面存放即可
	MarketBidAsk			bid_ask[9]; //一共10档买卖，第一档买卖放在base中了
}MarketDepthData;


typedef struct MarketOrderInfo
{
	__int64 nPrice;//'价格，单位1/10000元',
	unsigned int nTime; //'行情时间点 时分秒毫秒 如 093001500,表示09:30:01:500

	unsigned short nOrders;//'订单总数量',可能大于50，但后面最多记录50条
	unsigned char nSide; //'买卖方向(B:Bid A:Ask)',
	unsigned char nItems;//明细数量
	unsigned int nVolumes[50]; //订单明细
}MarketOrderInfo;

typedef struct MarketTransInfo
{
	__int64 			nPrice;//'价格，单位1/10000元',
	unsigned int 	nTime;   //'行情时间点 时分秒毫秒 如 093001500,表示09:30:01:500
	unsigned int 	nVolume; //'成交数量'
	char          chBS;    //买卖方向(买：B, 卖：A, 不明：)
}MarketTransInfo;

typedef struct MarketKlineInfo
{
	unsigned int date;
	unsigned int seq;
	unsigned long nTime;
	struct
	{
		__int64 nPriceOpen;  //开盘价
		__int64 nPriceClose; //收盘价
		__int64 nPriceHigh;  //最高
		__int64 nPriceLow;   //最低
		__int64 nVolume;    //成交量
		__int64 nTurnover;// 成交额
	}data;
}MarketKlineInfo;

typedef struct MarketAnalyseDataBase
{
	unsigned int  _date; //日期 YYYYMMDD,action day日期,实际交易发生日期
  unsigned int  _seq; //序列号,以交易日分割，每日清0
  int       	  _state; //0初始，1，修改中，2：完整，不再更改
	int				   _type;		//tag的类型
	__int64 		 _value;	//默认情况都有这几个数据
	__int64      _pv_total;
	__int64			 _volume_total;
	__int64      _pv_tick;
	unsigned int _volume_tick;
	unsigned int  OpenInterest;//持仓量
	unsigned int  _trade_date;//交易日期
	unsigned int	_time; //对应数据最新的行情时间（相对于交易发生日期的时间）
}MarketAnalyseDataBase;

typedef struct MarketAnalyseTagBase
{
	unsigned int _date;		//与主数据保持一致
	unsigned int _seq;		//与主数据保持一致
	short				 _type;		//tag的类型<0,=0,>0
	short				 _state;	//状态，用来控制是否最终状态，0为正式状态，其余为中间状态，
	int					 _param;	//其他参数，暂时未用，保留,0
	__int64 		 _value;	//默认情况都有这几个数据
}MarketAnalyseTagBase;

typedef struct MarketAnalyseKline
{
	struct
	{
		__int64 price;
		unsigned int date;
		unsigned int time;
		unsigned int seq; //对应的tick数据seq
	}open_item,close_item,max_item,min_item;
	//缠论的K线合并
	struct
	{
		__int64 price_max;
		__int64 price_min;
	}twist;

	__int64 diff;
	__int64 dea;
	__int64 bar;
	__int64 ema12;
	__int64 ema26;
}MarketAnalyseKline;

//中枢的扩展数据
typedef struct MarketAnalyseDpo
{
	unsigned int date;
	unsigned int seq;
	long long    price;
}MarketAnalyseDpo;

typedef struct HistoryRequest
{
	unsigned int date;
	unsigned int seq;
	int					 direct;
	unsigned int cnt;
	unsigned int block_size;
}HistoryRequest;

struct ContractInfo
{
	const char * id;      //
	const char * name;
	const char * exchange;//市场SH;SZ;DCE...
	const char * product; //产品
	unsigned int open_time;
	unsigned int close_time;

	long price_tick;    //每跳数值,原始值的10000倍，避免浮点运算
	long Multiple;      //每跳金额,原始值的10000倍，避免浮点运算
	long dyn_charge;    //手续费(万分比的10000倍)，避免浮点运算
	long fix_charge;    //固定手续费,金额的10000倍，避免浮点运算
};

//存放在每个文件的文件头
typedef struct ContractFileInfo
{
	char id[32];//合约ID
	char name[64];//合约名字，最大约20个汉字
	char exchange[32];//市场名称SH;SZ;DCE...
	char product[32];//产品名
	unsigned int open_time;
	unsigned int close_time;

	long long price_tick;    //每跳数值,原始值的10000倍，避免浮点运算
	long long Multiple;      //每跳金额,原始值的10000倍，避免浮点运算
	long long dyn_charge;    //手续费(万分比的10000倍)，避免浮点运算
	long long fix_charge;    //固定手续费,金额的10000倍，避免浮点运算
}ContractFileInfo;

typedef struct DiagramFileHead
{
	ContractFileInfo info;
	unsigned int block_size;
	unsigned int date;
	char 		class_name[64];
	char 		type_name[64];
	__int64 param;
}DiagramFileHead;


typedef struct DiagramTagFileHead
{
	DiagramFileHead  base;
	char 		p_class_name[64]; //数据描述
	char 		p_type_name[64];
	__int64 p_param;
}DiagramTagFileHead;

typedef struct DepthFileHead
{
	ContractFileInfo info;
	unsigned int block_size;
	unsigned int ms; //最小间隔周期，毫秒单位
	unsigned int date;
	unsigned int hour;
}DepthFileHead;

typedef struct DepthExtFileHead
{
	DepthFileHead dh;
	char hours[24];
	char reserve[8];
}DepthExtFileHead;

#pragma pack()


struct  MarketDataType
{
	unsigned long parent_index; //主数据的顺序，如果为-1，则自己为主数据，否则为tag
	unsigned long type_index; //该数据的顺序，在类型下从0开始，

	long  		   	param;		//数据级别,如5,10,30,60等
	const char *  name;		//数据名称,如秒，小时，分钟，天
	const char *  class_name;	//处理器,如k线
	short				  store_level; //存储周期级别，0按天，1按年,通过配置文件来配,默认按天,tag未配置情况下以其主数据周期为准
};



class MarketDataHandler;
class MarketDataItem;
class MarketDataTag;
class MarketHandler;

class MarketDataTriger
{
public:
	virtual void OnData(MarketDataHandler *handler,MarketDataItem * data,int mode) = 0;
	virtual void OnTag(const char * id,MarketDataType * dt,MarketDataTag * tag,int mode) = 0;
	//加载最新的cnt条数据
	virtual void LoadHistory(MarketHandler * h, int cnt,ContractInfo * contract,MarketDataType * data,MarketDataType * tag = 0) = 0;
	//从 unsigned int date,unsigned int seq开始加载后续所有数据
	virtual void LoadHistory(MarketHandler * h, unsigned int date,unsigned int seq,ContractInfo * contract,MarketDataType * data,MarketDataType * tag = 0) = 0;
};

#endif
