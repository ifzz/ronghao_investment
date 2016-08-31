#include "rhafx.h"


DiagramTag::DiagramTag()
{
	pri = 0;
}

DiagramTag::~DiagramTag()
{
	delete pri;
}


DiagramDataItem::DiagramDataItem(int cnt)
{
	pri = 0;

	need_write = 1;
	tags = 0;
	tag_cnt = cnt;
	if( tag_cnt > 0 )
	{
		tags = new DiagramTag*[tag_cnt];
		int i;
		for( i=0; i< tag_cnt; i++)
		{
			tags[i] = 0;
		}
	}
}

DiagramDataItem::~DiagramDataItem()
{
	if( tags )
	{
		int i;
		for( i=0; i< tag_cnt; i++)
		{
			delete tags[i];
		}

		delete [] tags;
	}
	delete pri;
}

void DiagramDataItem::tag_reset() {
	for (int i = 0; i < tag_cnt; ++i)
		delete tags[i];
}

DiagramTag * DiagramDataItem::PeekTag(int index)
{
	if( !tags )
		return 0;
	if( !tags[index] )
		return 0;
	if( tags[index]->base._date == 0 )
		return 0;
	return tags[index]; //这里没有判断越界的情况，所有调用者有保证不越界的责任
}

DiagramDataMgr::DiagramDataMgr(int create_factory  )
{
	factory = 0;
	if(create_factory )
	{
		CreateFactory();
	}
	m_params = 0;
}

DiagramDataMgr::~DiagramDataMgr()
{
	delete factory;
	delete m_params;
}

void DiagramDataMgr::CreateFactory()
{
	if( factory )
		return;

	factory = new DiagramDataFactory();
	factory->m_info = &info;

	factory->Init(g_data_tag_info.m_list);
}


static StockDataCache g_market_data_obj;
StockDataCache 	* g_market_data = &g_market_data_obj;

/*
void Diagram_OnFlushTimer(StockData * data,void * params)
{
	( (DiagramDataMgr*)data->data)->factory->doFlush(data->marketcode,data->code.c_str(),-1);
}

void Diagram_doFlush(unsigned long t)
{
	g_market_data->Each(Diagram_OnFlushTimer,0,E15_StockMarketCode_all);
}
*/

DiagramDataMgr * DiagramDataMgr_PeekData(int market,const char * id) //仅仅查找是否存在
{
	StockData * stock = g_market_data->PeekData(market,id);
	if( !stock )
		return 0;

	return (DiagramDataMgr *)stock->data;
}


DiagramDataMgr * DiagramDataMgr_GetData(int market,const char * id,const char * name,int create_factory) //如果数据不存在，则创建一个新的
{
	StockData * stock = g_market_data->GetData(market,id);
	if( stock->data )
	{
		if( name )
		{
			stock->name.Strcpy(name);
			((DiagramDataMgr *)stock->data)->info.name = stock->name.c_str();
		}
		return (DiagramDataMgr *)stock->data;
	}
	stock->name.Strcpy(name);
	stock->data = new DiagramDataMgr(create_factory);
	((DiagramDataMgr *)stock->data)->info.id = stock->code.c_str();
	((DiagramDataMgr *)stock->data)->info.name = stock->name.c_str();

	return (DiagramDataMgr *)stock->data;
}

void DiagramDataMgr_RemoveAll()
{
	g_market_data->Reset();
}


