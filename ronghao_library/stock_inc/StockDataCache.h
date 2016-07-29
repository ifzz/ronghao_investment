#ifndef __StockDataCache_H
#define __StockDataCache_H

#ifdef E15_market_analyse_export
        #ifndef E15_market_analyse_API
                #ifdef WIN32
                        #define E15_market_analyse_API __declspec(dllexport)
                #else
                        #define E15_market_analyse_API __attribute__((visibility("default")))
                #endif
        #endif
#else
        #ifndef E15_market_analyse_API
                #ifdef WIN32
                        #define E15_market_analyse_API __declspec(dllimport)
                #else
                        #define E15_market_analyse_API
                #endif
        #endif
#endif

#include "E15_string.h"


enum StockMarketCode
{
    E15_StockMarketCode_all = -1, //所有交易所
    E15_StockMarketCode_SZ = 0, //深圳交易所
    E15_StockMarketCode_SH, //上海交易所
    E15_StockMarketCode_HK, //香港交易所
    E15_StockMarketCode_SI,   //新加坡
    E15_StockMarketCode_CFuture,   //国内期货
    E15_StockMarketCode_Index,   //A股票指数, 6
    E15_StockMarketCode_CNA,
    E15_StockMarketCode_MAXCOUNT, //最大编号
};

E15_market_analyse_API const char *  GetMarketNameByCode(int code);
E15_market_analyse_API int MarketCodeById(const char * id);

class E15_market_analyse_API StockData
{
public:
    int            marketcode; //市场代码
    E15_String     code;  //股票代码
    E15_String     name; //股票名称
    E15_String 		 product;	//产品名称
    E15_String 		 exchange; //市场名称

    void *         data;

public:
	void SetData(void * data,void (*)(void *) );

public:
    StockData();
    virtual ~StockData();

private:
	void (*del_data)(void *);
};



class E15_market_analyse_API StockDataNode
{
public:
    StockDataNode **pNodeNumber;//数字
    StockDataNode **pNodeAlpha1;//大写
    StockDataNode **pNodeAlpha2;//小写

    StockData * data;

public:
    StockDataNode();
    ~StockDataNode();

    void Each( void (*f)(StockData * data,void * params), void * params );
    void Reset();
};


class E15_market_analyse_API MarketInfo
{
public:
    MarketInfo();
    ~MarketInfo();

    void Reset();

    StockDataNode   m_node; //目前先最大支持10个交易所
    E15_String      m_name; //市场名称
    E15_String      m_date; //
    E15_String      m_code;//

};

class E15_market_analyse_API StockDataCache
{
public:
    StockDataCache();
    ~StockDataCache();

    MarketInfo * GetMarketInfo(int market);

    StockData * GetData(int market, const char * code);
    StockData * PeekData(int market, const char * code);

    void Each( void (*f)(StockData * data,void * params), void * params ,int market = E15_StockMarketCode_all);

    void Reset(int market = -1);

private:

    MarketInfo * m_markets[E15_StockMarketCode_MAXCOUNT];//目前先最大支持10个交易所

    StockDataNode * GetNode(StockDataNode * node,char ch);
    StockDataNode * PeekNode(StockDataNode * node,char ch);
};



#endif
