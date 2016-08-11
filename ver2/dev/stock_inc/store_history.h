#ifndef __Store_History_H
#define __Store_History_H


#ifdef E15_store_history_export
        #ifndef E15_store_history_API
                #ifdef WIN32
                        #define E15_store_history_API __declspec(dllexport)
                #else
                        #define E15_store_history_API __attribute__((visibility("default")))
                #endif
        #endif
#else
        #ifndef E15_store_history_API
                #ifdef WIN32
                        #define E15_store_history_API __declspec(dllimport)
                #else
                        #define E15_store_history_API
                #endif
        #endif
#endif


#include "stock_data.h"
#include "E15_string.h"

union HistoryDiaBase
{
	MarketAnalyseDataBase * data_base;
	MarketAnalyseTagBase  * tag_base;
};

class E15_ValueTable;

typedef int (*History_OnDepth)(void * obj,int market,const char * id,MarketDepthBase * base,MarketDepthExt * ext,MarketBidAsk * ba);
typedef int (*History_OnDia)(void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,HistoryDiaBase * base,const char * ext_data,int len);

class E15_store_history_API E15_HistoryStore
{
public:
	E15_HistoryStore(){};
	virtual ~E15_HistoryStore(){};


	virtual void Init(const char * conf = "./ini/store.ini",E15_ValueTable * dia_desc = 0) = 0;

	//对于写操作，启动相关线程
	virtual void Start() = 0;
	virtual void Stop() = 0;//对于写操作，停止相关线程

	virtual void SaveInstrumentInfo(unsigned int date,int market,E15_ValueTable * vt) = 0;
	virtual void InitDepth(int market,ContractInfo * info) = 0;
	virtual void InitDia(int market,ContractInfo * info,MarketDataType * dt) = 0;

	virtual int SaveDepth(int market,const char * id,MarketDepthData * depth) = 0;
	virtual int SaveDiaData(int market,const char * id,MarketDataType * dt,MarketAnalyseDataBase * base,const char * ext_data,int len,int mode ) = 0;
	virtual int SaveDiaTag(int market,const char * id,MarketDataType * dt,MarketDataType * tt,MarketAnalyseTagBase * base,const char * ext_data,int len,int mode) = 0;

	//指定时间点加载指定数量，可以是前向，也可以是后向
	virtual int LoadDepthHistory(History_OnDepth f,void * obj,int market,const char * id,unsigned int date,unsigned int time,int cnt,int direct) = 0; //dir > 0 ，后向，<0 前向
	//把数据和tag组合成一个包
	virtual int LoadDiaHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,unsigned int date,unsigned int seq,int cnt,int direct) = 0; //dir > 0 ，后向，<0 前向
	//单独加载数据
	virtual int LoadDiaDataHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,unsigned int date,unsigned int seq,int cnt,int direct) = 0; //dir > 0 ，后向，<0 前向
	//单独加载tag
	virtual int LoadDiaTagHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,unsigned int date,unsigned int seq,int cnt,int direct) = 0;//dir > 0 ，后向，<0 前向

	//按天加载,如果date = 0,加载最早的日期，date = -1,加载最新日期
	virtual int LoadDepthHistory(History_OnDepth f,void * obj,int market,const char * id,unsigned int date) = 0; //dir > 0 ，后向，<0 前向
	//把数据和tag组合成一个包,加载的是该数据类型下的所有tag,tag没有指定type_index,
	virtual int LoadDiaHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,unsigned int date) = 0; //dir > 0 ，后向，<0 前向
	//单独加载数据
	virtual int LoadDiaDataHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,unsigned int date) = 0; //dir > 0 ，后向，<0 前向
	//单独加载tag
	virtual int LoadDiaTagHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,unsigned int date) = 0;//dir > 0 ，后向，<0 前向

	//加载指定范围内的指定tag
	virtual int LoadDiaTagHistory(History_OnDia f,void * obj,int market,const char * id,MarketDataType * dt,MarketDataType * tt,unsigned int date_b,unsigned int seq_b,unsigned int date_e,unsigned int seq_e) = 0;

};

extern "C"
{
E15_store_history_API E15_HistoryStore * Create_E15_HistoryStore();
E15_store_history_API void BuildListPath(E15_String * path, int marketcode,const char * id,const char * type);

E15_store_history_API void BuildDepthExt(E15_String * path, int marketcode,	const char * id, unsigned int day);
E15_store_history_API void BuildDepthPath(E15_String * path, int marketcode,	const char * id, unsigned int day,unsigned int hour); //hour 24小时制的小时

E15_store_history_API void BuildInstrumentListPath(E15_String * filepath, int marketcode,unsigned int date);
E15_store_history_API void BuildDiagramPath(E15_String * filepath, int marketcode,const char * id);
E15_store_history_API int BuildDiaDataPath(E15_String * path, int marketcode,	const char * id, unsigned int day,MarketDataType * dt);
E15_store_history_API int BuildDiaTagPath(E15_String * path, int marketcode,	const char * id, unsigned int day,MarketDataType * data,MarketDataType * tag);

E15_store_history_API int AppendDiaDataPath(E15_String * path,  unsigned int day,MarketDataType * dt);
E15_store_history_API int AppendDiaTagPath(E15_String * path,  unsigned int day,MarketDataType * data,MarketDataType * tag);

}


#endif
