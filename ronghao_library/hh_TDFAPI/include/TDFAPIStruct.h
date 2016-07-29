#ifndef __TDF_API_STRUCT_H__ 
#define __TDF_API_STRUCT_H__
#pragma  pack(push)
#pragma pack(1)


#ifndef THANDLE
typedef void* THANDLE;
#endif

#ifndef __int64
#define __int64 long long
#endif

//系统消息和数据消息的定义
enum TDF_MSG_ID
{
    MSG_INVALID = -100,

    //系统消息
    MSG_SYS_DISCONNECT_NETWORK,     //网络断开事件
    MSG_SYS_CONNECT_RESULT,         //主动发起连接的结果
    MSG_SYS_LOGIN_RESULT,           //登陆应答
    MSG_SYS_CODETABLE_RESULT,       //获取代码表结果
    MSG_SYS_QUOTATIONDATE_CHANGE,   //行情日期变更通知
    MSG_SYS_MARKET_CLOSE,           //闭市
    MSG_SYS_HEART_BEAT,             //服务器心跳消息

    //数据消息
    MSG_DATA_INDEX,                 //指数数据
    MSG_DATA_MARKET,                //行情数据
    MSG_DATA_FUTURE,                //期货行情
    MSG_DATA_TRANSACTION,           //逐笔成交
    MSG_DATA_ORDERQUEUE,            //委托队列
    MSG_DATA_ORDER,                 //逐笔委托
	MSG_DATA_HK_AMOUNT,             //港股通实时额度
	MSG_DATA_HK_STATUS,             //上交所港股通可接收订单并转发的产品状态数据

	MSG_SYS_PACK_OVER = -10,              //当前网络包解析完毕
};
//MSG_SYS_PACK_OVER
struct TDF_PACK_OVER
{
	int nDataNumber;
	int nConID;
};
//参考汇率一条记录
struct TDF_EXCHRATE_RECORD
{
	__int64    iBuyPrice;       //参考汇率买入价（小数点5位，乘以100000）
	__int64    iSellPrice;      //参考汇率卖出价（小数点5位，乘以100000）
	__int64    iMedianPrice;    //参考汇率中间价（小数点5位，乘以100000）
	char       chCurrencyType[8];   //货币种类
	//int        nDate;            //适用日期
};
//数据消息：MSG_DATA_HK_AMOUNT（港股通实时额度数据格式类型）
struct TDF_HK_AMOUNT_DATA
{
	int         nActionDay;             //业务发生日(自然日)
	int         nTradingDay;            //交易日
	int			nTime;					//时间(HHMMSSmmm)
	__int64     iThresholdAmount;       //每日初始额度，单位人民币元
	__int64     iPosAmt;	            //日中剩余额度，单位人民币元
	char        chAmountStatus;         //额度状态	1：额度用完；2：额度可用
	//参考汇率
	int         nExchRateItems;
	TDF_EXCHRATE_RECORD exchRates[2];
	//    int	nItemCount;	       //记录条数
	//    TDF_EXCHRATE_RECORD* pExchRates;
};

//数据消息：MSG_DATA_HK_STATUS（上交所港股通可接收订单并转发的产品状态数据）
struct TDF_HK_STATUS_DATA
{
	char        chCode[32];					    //证券代码
	char        chSecTradingStatus1[9];        //港股整手订单(C8):	该字段为8位字符串，左起每位表示特定的含义，无定义则填空格。第1位：‘0’表示限制买入，‘1’表示正常无此限制。
											   //第2位：‘0’表示限制卖出，‘1’表示正常无此限制。
	char        chSecTradingStatus2[9];        //港股零股订单(C8):	该字段为8位字符串，左起每位表示特定的含义，无定义则填空格。第1位：‘0’表示限制买入，‘1’表示正常无此限制。
											   //第2位：‘0’表示限制卖出，‘1’表示正常无此限制。		
};


//系统消息：MSG_SYS_CONNECT_RESULT 对应的结构
struct TDF_CONNECT_RESULT
{
    char szIp[32];
    char szPort[8];
    char szUser[32];
    char szPwd[32];

    unsigned int nConnResult; //为0则表示连接失败，非0则表示连接成功
    int nConnectionID;        //连接ID
};

//系统消息：MSG_SYS_LOGIN_RESULT 对应的结构
struct TDF_LOGIN_RESULT
{
    unsigned int nLoginResult;//为0则表示登陆验证失败，非0则表示验证成功

    char szInfo[128];       //登陆结果文本
    int nMarkets;           //市场个数
    char szMarket[256][8];  //市场代码 SZ, SH, CF, SHF, CZC, DCE
    int nDynDate[256];      //动态数据日期
};

//系统消息：MSG_SYS_CODETABLE_RESULT 对应的结构
struct TDF_CODE_RESULT
{
    char szInfo[128];       //代码表结果文本
    int nMarkets;           //市场个数
    char szMarket[256][8];  //市场代码
    int nCodeCount[256];    //代码表项数
    int nCodeDate[256];     //代码表日期

};

//系统消息：MSG_SYS_QUOTATIONDATE_CHANGE 对应的结构
struct TDF_QUOTATIONDATE_CHANGE
{
    char szMarket[8];	    //市场代码
    int nOldDate;	        //原行情日期
    int nNewDate;	        //新行情日期
};

//系统消息：MSG_SYS_MARKET_CLOSE 对应的结构
struct TDF_MARKET_CLOSE
{
    char    szMarket[8];        //交易所名称
    int		nTime;				//时间(HHMMSSmmm)
    char	chInfo[64];			//闭市信息
};


struct TDF_CODE
{
    char szWindCode[32];    //Wind Code: AG1302.SHF
    char szMarket[8];       //market code: SHF
    char szCode[32];        //original code:ag1302
    char szENName[32];
    char szCNName[32];      //chinese name: 沪银1302
    int nType;                            
};

struct TDF_OPTION_CODE
{
    TDF_CODE basicCode;
    
    char szContractID[32];// 期权合约代码
    char szUnderlyingSecurityID[32];//// 标的证券代码
    char chCallOrPut;               // 认购认沽C1        认购，则本字段为“C”；若为认沽，则本字段为“P”
    int  nExerciseDate;             // 期权行权日，YYYYMMDD
    
    //扩充字段
    char chUnderlyingType;			// 标的证券类型C3    0-A股 1-ETF (EBS – ETF， ASH – A 股)
	char chOptionType;              // 欧式美式C1        若为欧式期权，则本字段为“E”；若为美式期权，则本字段为“A”
	
	char chPriceLimitType;          // 涨跌幅限制类型C1 ‘N’表示有涨跌幅限制类型, ‘R’表示无涨跌幅限制类型
	int  nContractMultiplierUnit;	// 合约单位,         经过除权除息调整后的合约单位, 一定是整数
	int  nExercisePrice;            // 期权行权价,       经过除权除息调整后的期权行权价，右对齐，精确到厘
	int  nStartDate;                // 期权首个交易日,YYYYMMDD
	int  nEndDate;                  // 期权最后交易日/行权日，YYYYMMDD
	int  nExpireDate;               // 期权到期日，YYYYMMDD
};
union TD_EXCODE_INFO
{
	struct TD_OptionCodeInfo            //futures options 专用 (nType >= 0x90 && nType <= 0x95),非期权下列字段无效
	{
		char chContractID[32];           // 期权合约代码C19
		char chUnderlyingSecurityID[32]; // 标的证券代码
		char chUnderlyingType;			// 标的证券类型C3    0-A股 1-ETF (EBS – ETF， ASH – A 股)
		char chOptionType;              // 欧式美式C1        若为欧式期权，则本字段为“E”；若为美式期权，则本字段为“A”
		char chCallOrPut;               // 认购认沽C1        认购，则本字段为“C”；若为认沽，则本字段为“P”
		char chPriceLimitType;          // 涨跌幅限制类型C1 ‘N’表示有涨跌幅限制类型, ‘R’表示无涨跌幅限制类型
		int  nContractMultiplierUnit;	// 合约单位,         经过除权除息调整后的合约单位, 一定是整数
		int  nExercisePrice;            // 期权行权价,       经过除权除息调整后的期权行权价，右对齐，精确到厘
		int  nStartDate;                // 期权首个交易日,YYYYMMDD
		int  nEndDate;                  // 期权最后交易日/行权日，YYYYMMDD
		int  nExerciseDate;             // 期权行权日，YYYYMMDD
		int  nExpireDate;               // 期权到期日，YYYYMMDD
	}Option;
};
struct TDF_CODE_INFO
{
	//int  nIdnum;				//本日编号(交易所编号*100 + 交易所编号)
	int  nType;					//证券类型
	char chCode[32];            //证券代码
	char chName[64];			//汉语证券名称
	
	TD_EXCODE_INFO exCodeInfo;
};

struct TDF_APP_HEAD
{
    int	nHeadSize;         //本记录结构大小
    int	nItemCount;	       //记录条数
    int	nItemSize;         //记录大小
};

struct TDF_MSG
{
    unsigned short  	    sFlags;		        //16位 标识符,升级到 TDF_VERSION_NX_START_6001 .
    int  	                nDataType;	        //16位 数据类型
    int			            nDataLen;	        //32位 数据长度（不包括TDF_APP_HEAD的长度）
    int			            nServerTime;		//32位服务器时间戳（精确到毫秒HHMMSSmmm），对于系统消息为0
    int     		        nOrder;		        //32位 流水号
    int                     nConnectId;         //连接ID，在TDF_Open设置
    TDF_APP_HEAD*           pAppHead;	        //应用头
    void*                   pData;              //数据指针
};

typedef void (*TDF_DataMsgHandler)(THANDLE hTdf, TDF_MSG* pMsgHead);   //数据回调，用于通知用户收到了行情、逐笔成交，逐笔委托，委托队列等,pMsgHead->pAppHead->ItemCount字段可以获知得到了多少条记录，pMsgHead->pAppHead->pData指向第一条数据记录


typedef void (*TDF_SystemMsgHandler)(THANDLE hTdf, TDF_MSG* pMsgHead); //系统消息回调，用于通知用户收到了网络断开事件、连接（重连）结果、代码表结果等。当获取系统消息时，pMsgHead->pAppHead指针为空, pMsgHead->pData指向相应的结构体


//数据消息：MSG_DATA_MARKET 对应的结构
struct TDF_MARKET_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int			 nTime;					//时间(HHMMSSmmm)
    int			 nStatus;				//状态
    unsigned int nPreClose;				//前收盘价
    unsigned int nOpen;					//开盘价
    unsigned int nHigh;					//最高价
    unsigned int nLow;					//最低价
    unsigned int nMatch;				//最新价
    unsigned int nAskPrice[10];			//申卖价
    unsigned int nAskVol[10];			//申卖量
    unsigned int nBidPrice[10];			//申买价
    unsigned int nBidVol[10];			//申买量
    unsigned int nNumTrades;			//成交笔数
    __int64		 iVolume;				//成交总量
    __int64		 iTurnover;				//成交总金额
    __int64		 nTotalBidVol;			//委托买入总量
    __int64		 nTotalAskVol;			//委托卖出总量
    unsigned int nWeightedAvgBidPrice;	//加权平均委买价格
    unsigned int nWeightedAvgAskPrice;  //加权平均委卖价格
    int			 nIOPV;					//IOPV净值估值
    int			 nYieldToMaturity;		//到期收益率
    unsigned int nHighLimited;			//涨停价
    unsigned int nLowLimited;			//跌停价
    char		 chPrefix[4];			//证券信息前缀
    int			 nSyl1;					//市盈率1
    int			 nSyl2;					//市盈率2
    int			 nSD2;					//升跌2（对比上一笔）
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};

//数据消息：MSG_DATA_INDEX 对应的结构
struct TDF_INDEX_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int         nTime;			//时间(HHMMSSmmm)
    int		    nOpenIndex;		//今开盘指数
    int 	    nHighIndex;		//最高指数
    int 	    nLowIndex;		//最低指数
    int 	    nLastIndex;		//最新指数
    __int64	    iTotalVolume;	//参与计算相应指数的交易数量
    __int64	    iTurnover;		//参与计算相应指数的成交金额
    int		    nPreCloseIndex;	//前盘指数
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};


//数据消息：MSG_DATA_FUTURE 对应的结构
struct TDF_FUTURE_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int			 nTime;					//时间(HHMMSSmmm)	
    int			 nStatus;				//状态
    __int64		 iPreOpenInterest;		//昨持仓
    unsigned int nPreClose;				//昨收盘价
    unsigned int nPreSettlePrice;		//昨结算
    unsigned int nOpen;					//开盘价	
    unsigned int nHigh;					//最高价
    unsigned int nLow;					//最低价
    unsigned int nMatch;				//最新价
    __int64		 iVolume;				//成交总量
    __int64		 iTurnover;				//成交总金额
    __int64		 iOpenInterest;			//持仓总量
    unsigned int nClose;				//今收盘
    unsigned int nSettlePrice;			//今结算
    unsigned int nHighLimited;			//涨停价
    unsigned int nLowLimited;			//跌停价
    int			 nPreDelta;			    //昨虚实度
    int			 nCurrDelta;            //今虚实度
    unsigned int nAskPrice[5];			//申卖价
    unsigned int nAskVol[5];			//申卖量
    unsigned int nBidPrice[5];			//申买价
    unsigned int nBidVol[5];			//申买量

	//Add 20140605
	int	lAuctionPrice;		//波动性中断参考价
	int	lAuctionQty;		//波动性中断集合竞价虚拟匹配量
	//Add 20141014
	int    lAvgPrice;          //郑商所期货均价
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};

//数据消息：MSG_DATA_ORDERQUEUE 对应的结构
struct TDF_ORDER_QUEUE
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //原始Code
    int     nActionDay;     //自然日
    int 	nTime;			//时间(HHMMSSmmm)
    int     nSide;			//买卖方向('B':Bid 'A':Ask)
    int		nPrice;			//委托价格
    int 	nOrders;		//订单数量
    int 	nABItems;		//明细个数
    int 	nABVolume[200];	//订单明细
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};

//数据消息：MSG_DATA_TRANSACTION 对应的结构
struct TDF_TRANSACTION
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //原始Code
    int     nActionDay;     //自然日
    int 	nTime;		    //成交时间(HHMMSSmmm)
    int 	nIndex;		    //成交编号
    int		nPrice;		    //成交价格
    int 	nVolume;	    //成交数量
    int		nTurnover;	    //成交金额
    int     nBSFlag;        //买卖方向(买：'B', 卖：'A', 不明：' ')
    char    chOrderKind;    //成交类别
    char    chFunctionCode; //成交代码
    int	    nAskOrder;	    //叫卖方委托序号
    int	    nBidOrder;	    //叫买方委托序号
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};

//数据消息：MSG_DATA_ORDER 对应的结构
struct TDF_ORDER
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //原始Code
    int 	nActionDay;	    //委托日期(YYMMDD)
    int 	nTime;			//委托时间(HHMMSSmmm)
    int 	nOrder;	        //委托号
    int		nPrice;			//委托价格
    int 	nVolume;		//委托数量
    char    chOrderKind;	//委托类别
    char    chFunctionCode;	//委托代码('B','S','C')
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};


enum DATA_TYPE_FLAG
{
    DATA_TYPE_ALL = 0,//除掉DATA_TYPE_FUTURE_CX 以外的所有数据
    
    DATA_TYPE_INDEX = 0x1,  //指数
    DATA_TYPE_TRANSACTION = 0x2,//逐笔成交
    DATA_TYPE_ORDER = 0x4, //逐笔委托
    DATA_TYPE_ORDERQUEUE=0x8,//委托队列
    
    DATA_TYPE_FUTURE_CX = 0x10,//期货连线
};

enum TDF_PROXY_TYPE
{
    TDF_PROXY_SOCK4,
    TDF_PROXY_SOCK4A,
    TDF_PROXY_SOCK5,
    TDF_PROXY_HTTP11,
};

struct TDF_PROXY_SETTING
{
    TDF_PROXY_TYPE nProxyType;
    char szProxyHostIp[32];
    char szProxyPort[8];
    char szProxyUser[32];
    char szProxyPwd[32];
};

struct TDF_OPEN_SETTING
{
    char szIp[32];
    char szPort[8];
    char szUser[32];
    char szPwd[32];

    unsigned int nReconnectCount;   //当连接断开时重连次数
    unsigned int nReconnectGap;     //重连间隔
    
    TDF_DataMsgHandler pfnMsgHandler;       //数据消息处理回调
    TDF_SystemMsgHandler pfnSysMsgNotify;   //系统消息通知回调

    unsigned int nProtocol; //协议号，为0则为默认，或者TDF_VERSION_CURRENT
    const char* szMarkets;  //例如"SZ;SH;CF;SHF;DCE;SHF"，需要订阅的市场列表，以“;”分割,为空则订阅所有支持的市场
    const char* szSubScriptions; //例如"600000.sh;ag.shf;000001.sz"，需要订阅的股票，以“;”分割，为空则订阅全市场
    unsigned int nDate;     //请求的日期，格式YYMMDD，为0则请求今天
    unsigned int nTime;     //请求的时间，格式HHMMSS，为0则请求实时行情，为0xffffffff从头请求
    unsigned int nTypeFlags;    //为0请求所有品种，或者取值为DATA_TYPE_FLAG中多种类别，比如DATA_TYPE_MARKET | DATA_TYPE_TRANSACTION

    unsigned int nConnectionID; //连接ID，连接回调消息的附加结构 TDF_CONNECT_RESULT中 会包含这个ID
	int nInDex; //第几个打开的市场。
};

// Connect Server Info.
struct TDF_SERVER_INFO
{
	char szIp[32];
	char szPort[8];
	char szUser[32];
	char szPwd[32];
};

// TDF_OPEN_SETTING_EXT 
//   增加连接服务器，确保数据能够正确获得。
struct TDF_OPEN_SETTING_EXT
{
	TDF_SERVER_INFO	siServer[4];

	unsigned int nReconnectCount;   //当连接断开时重连次数
	unsigned int nReconnectGap;     //重连间隔

	TDF_DataMsgHandler pfnMsgHandler;       //数据消息处理回调
	TDF_SystemMsgHandler pfnSysMsgNotify;   //系统消息通知回调

	unsigned int nProtocol; //协议号，为0则为默认，或者0x6001
	const char* szMarkets;  //例如"SZ;SH;CF;SHF;DCE;SHF"，需要订阅的市场列表，以“;”分割,为空则订阅所有支持的市场
	const char* szSubScriptions; //例如"600000.sh;ag.shf;000001.sz"，需要订阅的股票，以“;”分割，为空则订阅全市场
	unsigned int nDate;     //请求的日期，格式YYMMDD，为0则请求今天
	unsigned int nTime;     //请求的时间，格式HHMMSS，为0则请求实时行情，为0xffffffff从头请求
	unsigned int nTypeFlags;    //为0请求所有品种，或者取值为DATA_TYPE_FLAG中多种类别，比如DATA_TYPE_MARKET | DATA_TYPE_TRANSACTION

	unsigned int nConnectionID; //连接ID，连接回调消息的附加结构 TDF_CONNECT_RESULT中 会包含这个ID
};

//环境设置，在调用TDF_Open之前设置
enum TDF_ENVIRON_SETTING
{
    TDF_ENVIRON_HEART_BEAT_INTERVAL,    //Heart Beat间隔（秒数）, 若值为0则表示默认值10秒钟
    TDF_ENVIRON_MISSED_BEART_COUNT,     //如果没有收到心跳次数超过这个值，且没收到其他任何数据，则判断为掉线，若值0为默认次数2次
    TDF_ENVIRON_OPEN_TIME_OUT,          //在调TDF_Open期间，接收每一个数据包的超时时间（秒数，不是TDF_Open调用总的最大等待时间），若值为0则默认30秒
	TDF_ENVIRON_OUT_LOG,
	TDF_ENVIRON_NOTTRANS_OLD_DATA, //通知服务器不要传送昨天的数据
	TDF_ENVIRON_USE_PACK_OVER,
};


enum SUBSCRIPTION_STYLE
{
    SUBSCRIPTION_FULL = 3, //全市场订阅
    SUBSCRIPTION_SET=0, //设置为订阅列表中股票，
    SUBSCRIPTION_ADD=1,  //增加订阅列表中股票
    SUBSCRIPTION_DEL=2,   //删除列表中的订阅
};

#pragma pack(pop)
#endif
