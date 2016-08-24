#ifndef __Data_Mgr_H
#define __Data_Mgr_H

#include "E15_map.h"
#include "E15_queue.h"
#include "E15_string_array.h"
#include "E15_value.h"
#include "E15_thread.h"
#include "StockDataCache.h"
#include "stock_data.h"

#include <deque>
#include <set>
#include <memory>

class DiagramDataItem;
struct raw_dia_group {
	DiagramDataItem *data;
	int data_index, mode;
	std::map<int, int> tag_mode;		//it->first: tag_index, it->second: tag_mode

	raw_dia_group()
	:data(nullptr)
	,mode(-1) {}
};
extern std::deque<raw_dia_group> g_dia_deq;

extern StockDataCache 	* g_market_data;

class DiagramTag
{
public:
	MarketAnalyseTagBase base;	//基本数据
	E15_String *pri;						//扩展数据，根据类型而不同

public:
	DiagramTag();
	~DiagramTag();

private:
	friend class DiagramDataFactory;
};

class DiagramDataItem : public E15_QueueItem
{
public:
	MarketAnalyseDataBase base;	//基本数据
	E15_String 						*pri;	//扩展数据，根据类型而不同

	DiagramTag * PeekTag(int index);

public:
	DiagramDataItem(int tag_cnt);
	virtual ~DiagramDataItem();


public:
	DiagramTag ** tags;
	int			  tag_cnt;

	short need_write;

	friend class HistoryStoreHelper;
	friend class DiagramDataFactory;
	friend class DiagramDataHandler;

};


class DataDescription : public E15_QueueItem
{
public:
	MarketDataType m_dt;
	E15_Queue * m_sub;

	char class_name[64];	//处理器,如k线
	char name[64];		//数据名称,如秒，小时，分钟，天


public:
	DataDescription();
	virtual ~DataDescription();
};


class DataDescriptionMgr
{
public:
	DataDescriptionMgr();
	~DataDescriptionMgr();

	E15_Queue *    m_list;

public:
	void InitDescription(const char *data, size_t len);
	void InitDescription(E15_ValueTable * vt);


};

class DiagramDataHandler ;
class DiagramDataFactory
{
public:
	DiagramDataFactory( );
	~DiagramDataFactory();
	void Init(E15_Queue * ds ); //数据描述信息
	void Reset(); //清理数据，避免内存占用太多
public:
	DiagramDataHandler * GetDataHandler(int index);

	int OnData(MarketDepthData * depth,int mode,int index,E15_String * vt); 	//网络实时发送的数据
	int OnTag(MarketDepthData * depth,int mode,int data_index,int tag_index,E15_String * vt);		//网络实时发送的数据

	int LoadHistoryData(HistoryRequest * req,const char * data,unsigned int len,int index ,int more_data);//加载历史数据
	int LoadHistoryTag(HistoryRequest * req,const char * data,unsigned int len,int index,int parent_index ,int more_data);//加载历史数据

	int LoadCacheData(const char * data,unsigned int len,int index ,int block_size);//加载最新缓存数据
	int LoadCacheTag(const char * data,unsigned int len,int index,int parent_index ,int block_size);//加载最新缓存数据

	ContractInfo 		*m_info;
public:
	E15_Queue * 	m_data;

	int OnData(MarketDepthData * depth,int mode,int index,MarketAnalyseDataBase * base,const char * ext_data,int len); 	//网络实时发送的数据
	int OnTag(MarketDepthData * depth,int mode,int data_index,int tag_index,MarketAnalyseTagBase * base,const char * ext_data,int len);		//网络实时发送的数据

	friend int factory_on_data(DataDescription * desc, DiagramDataFactory * factory);
	friend class HistoryStoreHelper;
};

class E15_Zip;
class DiagramHanlderParam;
//保留最新的部分数据以及需要显示的完整数据，其余部分存放在磁盘动态加载，避免内存需求过高
class DiagramDataHandler : public E15_QueueItem
{
public:

	int GetTagCount();

	MarketDataType * GetDataType();
	MarketDataType * GetTagType(int index);
	DiagramDataItem * PeekDataItem(int offset);

	E15_Queue * PeekData();

	E15_Queue *	PeekSub();

	void doFlush(int market,const char * id,int force_all = 0);

	void BuildHistoryReq(E15_String * req); //请求的历史数据格式

	MarketDataType * DataType();

	int SaveData(E15_Zip * zip);
	int SaveTag(int index,E15_Zip * zip);

private:
	DiagramDataItem * FindData(unsigned int  date,unsigned int seq);

	void doWriteData(E15_String * basedir,int force = 0);
	void doWriteTag(MarketDataType * tag_type,E15_String * basedir,int index,int force = 0);



public:
	E15_Queue * 	m_data; 	//待显示绘制数据
	E15_Queue *		m_sub;		//tag的类型描述

	MarketDataType * m_dt;

	DiagramDataItem * m_write_item; //下一个需要存储数据的开始位置
	int 				m_write_flag; //是否已经写入磁盘的标志
	unsigned int m_write_date;
	unsigned int m_write_seq;
	unsigned int m_hash; //数据堆积随机散列，避免同时磁盘操作

	long m_write_pos;
	E15_String * m_write_cache;

	unsigned int m_cache_date;
	unsigned int m_cache_seq;


public:
	short						m_history_over; //当没有更多历史数据时
	unsigned int 		ext_len; //扩展数据长度

	DiagramDataHandler();
	virtual ~DiagramDataHandler();

	friend class DiagramDataFactory;
	friend int factory_on_tag(DataDescription * desc, DiagramDataHandler * h);
	friend int factory_on_data(DataDescription * desc, DiagramDataFactory * factory);
	friend int handler_diagram_item(E15_Key * key,E15_Value * info,DiagramHanlderParam * p);
	friend class HistoryStoreHelper;
};


//--------------------------------------------------------------------

class DiagramDataMgrParams
{
public:
	DiagramDataMgrParams(){};
	virtual ~DiagramDataMgrParams(){};
};

class DiagramDataMgr
{
public:
	ContractInfo 			info;		//合约信息
	DiagramDataFactory 		*factory;    //数据工厂

	DiagramDataMgrParams * m_params;

	E15_ValueTable  m_vt;
	MarketDepthData * depth;

	E15_Lock lock;
	void CreateFactory();

public:
	DiagramDataMgr(int create_factory  );
	virtual ~DiagramDataMgr();
};


DiagramDataMgr * DiagramDataMgr_PeekData(int market,const char * id); //仅仅查找是否存在
DiagramDataMgr * DiagramDataMgr_GetData(int market,const char * id,const char * name,int create_factory ); //如果数据不存在，则创建一个新的

void Diagram_doFlush(unsigned long );
void DiagramDataMgr_RemoveAll();

extern DataDescriptionMgr g_data_tag_info;

#endif
