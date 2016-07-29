#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "E15_debug.h"
#include "E15_file.h"

#include "client_mgr.h"
#include "E15_ini.h"

#include "stock_msg.h"
#include "stock_data.h"
#include "StockDataCache.h"

#include "instrument.h"

Client_Mgr * g_server = 0;

Client_Mgr::Client_Mgr()
{
	m_subscribe_all = new E15_Intmap;
	m_cache = new StockDataCache;
	m_forward_cnt = 0;

}

Client_Mgr::~Client_Mgr()
{
	delete m_subscribe_all;
	delete m_cache;
}

int Client_Mgr::OnOpen(E15_ServerInfo * info,E15_String *&  json)
{
	E15_Debug::Printf(0,"[%x:%x] (N=%d,name=%s:role=%s) 上线\n",info->id.h,info->id.l,info->N,info->name,info->role);
	E15_Debug::Log(0,"[%x:%x] (N=%d,name=%s:role=%s) 上线\n",info->id.h,info->id.l,info->N,info->name,info->role);
	//有行情服务器或者指标订阅客户端
	if( info->N == 0 ) //指标订阅客户端,需要把服务器已有信息发送给客户端
	{
		info->user_obj = new E15_Strmap;

		//首先发送指标描述信息
		E15_ServerCmd  cmd;

		if( m_instrument_list_buffer.Length() > 0 )
		{
			cmd.cmd = Stock_Msg_InstrumentList;
			E15_Debug::Printf(0,"[%x:%x] Stock_Msg_InstrumentList 发送合约列表信息 to (N=%d,name=%s:role=%s)\n",info->id.h,info->id.l,info->N,info->name,info->role);
			Notify(&info->id,0,&cmd,m_instrument_list_buffer.c_str(),m_instrument_list_buffer.Length(),1 );
		}

		if( m_diagram_info.Length() > 0 )
		{
			cmd.cmd = Stock_Msg_DiagramInfo; //指标描述信息
			cmd.status = 0;
			E15_Debug::Printf(0,"[%x:%x] 发送指标描述信息 to (N=%d,name=%s:role=%s)\n",info->id.h,info->id.l,info->N,info->name,info->role);
			Notify(&info->id,0,&cmd,m_diagram_info.c_str(),m_diagram_info.Length(),1 );
		}
		/*
		if( m_diagram_list.Length() > 0 )
		{
			cmd.cmd = Stock_Msg_DiagramList;
			Notify(&info->id,0,&cmd,m_diagram_list.c_str(),m_diagram_list.Length(),1);
		}*/
		return 1;
	}

	return 1;
}

int get_market_by_id(const char * id)
{
	if( id[0] > '9' )
		return E15_StockMarketCode_CFuture;

	if( id[0] < '0' )
		return E15_StockMarketCode_CFuture;

	if( id[0] == '6' )
		return E15_StockMarketCode_SH;

	return E15_StockMarketCode_SZ;
}

int del_subscribe_item(const char * ins_id,unsigned long ukey,E15_ServerInfo * info,StockDataCache * cache)
{
	StockData * data;
	int market = get_market_by_id(ins_id );
	data = cache->PeekData(market,ins_id);
	if( !data )
		return 0;
	if( !data->data )
		return 0;

	((InstrumentInfo *)data->data)->m_subscribe_hash->Remove2(info->id.h,info->id.l);

	return 0;
}

int Client_Mgr::OnClose(E15_ServerInfo * info)
{
	E15_Debug::Printf(0,"[%x:%x] (N=%d,name=%s:role=%s) 下线\n",info->id.h,info->id.l,info->N,info->name,info->role);
	E15_Debug::Log(0,"[%x:%x] (N=%d,name=%s:role=%s) 下线\n",info->id.h,info->id.l,info->N,info->name,info->role);

	//首先删除可能的订阅标记
	if( info->N != 0 )
		return -1;

	m_subscribe_all->Remove2(info->id.h,info->id.l); //特殊标记
	//从合约列表中删除个性订阅
	E15_Strmap * subscribe_list = (E15_Strmap * )info->user_obj;

	subscribe_list->each((int (*)(const char * ,unsigned long ,void * ,void * ))del_subscribe_item,m_cache);

	delete subscribe_list;
	info->user_obj = 0;
	return -1;
}

int Client_Mgr::HandleSubscribeById(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json)
{
	//先判断是否已经订阅了所有数据
	if( m_subscribe_all->Lookup2(info->id.h,info->id.l) )
		return 0;

	int c = 0;
	E15_Value * v;
	E15_StringArray * sa = 0;

	E15_ValueTable vt;
	vt.Import(json->c_str(),json->Length() );
	v = vt.ValueS("id_list");
	if( !v )
	{
		E15_Debug::Printf(0,"HandleSubscribeById error, not find id_list, req data len=%d\n",json->Length());
		vt.Print();
		return -1;
	}
	sa = v->GetStringArray();
	c = sa->Size();
	if( c == 0 )
	{
		E15_Debug::Printf(0,"HandleSubscribeById error, 没指定合约\n");
		vt.Print();
		return -1;
	}

	int i;
	E15_String * s;
	InstrumentInfo * pMdInfo;
	E15_Strmap * subscribe_list = (E15_Strmap * )info->user_obj;
	StockData * data;

	for( i=0; i<c; i++ )
	{
		s = sa->At(i);
		int market = get_market_by_id(s->c_str() );
		data = m_cache->PeekData(market,s->c_str() );
		if( !data )
			continue;
		if( !data->data )
			continue;


		pMdInfo = (InstrumentInfo *)data->data;
		if(cmd->cmd == Stock_Msg_UnSubscribeById ) //取消订阅
		{
			E15_Debug::Printf(0,"[%x:%x] 取消订阅 %s\n",info->id.h,info->id.l,s->c_str() );
			pMdInfo->m_subscribe_hash->Remove2(info->id.h,info->id.l);
			subscribe_list->RemoveS(s->c_str() );
		}
		else
		{
			E15_Debug::Printf(0,"[%x:%x] 开始订阅 %s\n",info->id.h,info->id.l,s->c_str() );
			pMdInfo->m_subscribe_hash->SetAt2(info,info->id.h,info->id.l);
			subscribe_list->SetAtS(info,s->c_str() );
		}
	}
	return c;
}

void Client_Mgr::HandleSubscribeAll(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json)
{
	E15_Debug::Printf(0,"HandleSubscribeAll::cmd=%u\n",cmd->cmd);
	//从合约列表中删除订阅(个别的)
	//无论是订阅还是取消，都应该把个别订阅的先取消，避免重复
	E15_Strmap * subscribe_list = (E15_Strmap * )info->user_obj;
	subscribe_list->each((int (*)(const char * ,unsigned long ,void * ,void * ))del_subscribe_item,m_cache);

	if(cmd->cmd == Stock_Msg_UnSubscribeAll ) //取消
		m_subscribe_all->Remove2(info->id.h,info->id.l); //特殊标记
	else
		m_subscribe_all->SetAt2(info,info->id.h,info->id.l);
}

void Client_Mgr::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json)
{
	//订阅指定合约，取消订阅指定合约，获取历史数据
	E15_Debug::Printf(0,"OnRequest::cmd=%u,len=%d\n",cmd->cmd,json->Length());
	switch(cmd->cmd)
	{
	case Stock_Msg_SubscribeAll://取消所有合约订阅
	case Stock_Msg_UnSubscribeAll://订阅所有合约订阅
		HandleSubscribeAll(info,rt,cmd,json);
		break;
	case Stock_Msg_SubscribeById: //取消指定合约的订阅
	case Stock_Msg_UnSubscribeById: //订阅指定合约
		HandleSubscribeById(info,rt,cmd,json);
		break;
	default:
		break;
	}

	Response(&info->id,0,cmd,json);
}

void Client_Mgr::OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json)
{
	E15_Debug::Log(0,"[%x:%x] 响应(cmd=%u,seq=%u,status=%d)\n",info->id.h,info->id.l,cmd->cmd,cmd->seq,cmd->status);
}

struct instrument_list_each_param
{
	int market;
	StockDataCache * cache;
	E15_ValueTable   * ins_list;
	E15_ServerInfo * info;
	int refresh;
};

int instrument_list_each(E15_Key * key,E15_Value *info,instrument_list_each_param * p)
{
	const char * id = key->GetS();
	if( !id )
		return 0;
	const char * name = info->BytesS("name",0);
	int market = get_market_by_id(id );


	StockData * data = p->cache->GetData(market,id);
	if( data->data )
		return 0;
	data->name.Strcpy(name);
	InstrumentInfo * mgr = new InstrumentInfo;
	mgr->info.id = data->code.c_str();
	mgr->info.name = data->name.c_str();
	mgr->info.price_tick = info->BaseS("tick");
	mgr->info.Multiple = info->BaseS("Multiple");
	mgr->m_depth = p->info->id;

	E15_ValueTable * vt = p->ins_list->InsertTableS(id);
	vt->SetSI("market",data->marketcode);
	vt->SetSS("name",mgr->info.name );
	vt->SetSI("tick",mgr->info.price_tick);
	vt->SetSI("Multiple",mgr->info.Multiple);
	p->refresh = 1;
//	vt->SetSS("exchange",product->exchange.c_str() );
//	vt->SetSS("product",product->id.c_str());

	data->SetData(mgr,(void (*)(void *) )del_instrument);

	return 0;
}

struct forward_param
{
	Client_Mgr * mgr;
	E15_ServerCmd * cmd;
	E15_String * json;
};

int do_forward(unsigned long key1,unsigned long key2,unsigned long key3,E15_ServerInfo * info,forward_param * p)
{
	p->mgr->Notify(&info->id,0,p->cmd,p->json->c_str(),p->json->Length());
	return 0;
}


void Client_Mgr::OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json)
{
	//E15_Debug::Printf(0,"OnNotify::cmd=%u,len=%d\n",cmd->cmd,json->Length());
	if( info->N == 0 )
		return;

	E15_Id id;
	id.l = 0;
	id.h = 0x1; //给所有客户端节点广播

	if( cmd->cmd == Stock_Msg_InstrumentList )
	{
		E15_Debug::Printf(0,"OnNotify::cmd=%u,len=%d\n",cmd->cmd,json->Length());
		//合约列表
		E15_String s;
		m_zip.unzip_start(&s);
		m_zip.unzip(json->c_str(),json->Length() );
		m_zip.unzip_end();

		m_vt.Import(s.c_str(),s.Length() );

		instrument_list_each_param p;
		p.market = cmd->status;
		p.cache = m_cache;
		p.ins_list = &m_instrument_list;
		p.refresh = 0;

		m_vt.each( (int (*)(E15_Key *,E15_Value *,void * ))instrument_list_each,&p);

		//发送给所有客户端
		if( !p.refresh )
			return;

		m_instrument_list_buffer.Reset();
		s.Reset();
		m_instrument_list.Dump(&s);
		m_zip.zip_start(&m_instrument_list_buffer);
		m_zip.zip(s.c_str(),s.Length());
		m_zip.zip_end();

		cmd->cmd = Stock_Msg_InstrumentList;

		cmd->type = 0xff;
		E15_Debug::Printf(0,"Broadcast Stock_Msg_InstrumentList,len=%ld\n",m_instrument_list_buffer.Length());
		Notify(&id,0,cmd,m_instrument_list_buffer.c_str(),m_instrument_list_buffer.Length(),1);

		return ;
	}



	switch( cmd->cmd )
	{
	case Stock_Msg_DiagramInfo:
		m_diagram_info.Memcpy(json->c_str(),json->Length());
		E15_Debug::Printf(0,"Broadcast Stock_Msg_DiagramInfo,len=%ld\n",m_diagram_info.Length());
		cmd->type = 0xff;
		Notify(&id,0,cmd,m_diagram_info.c_str(),m_diagram_info.Length());
		return;
	case Stock_Msg_DiagramList:
		m_diagram_list.Memcpy(json->c_str(),json->Length());
		E15_Debug::Printf(0,"Broadcast Stock_Msg_DiagramList,len=%ld\n",m_diagram_list.Length());
		cmd->type = 0xff;
		Notify(&id,0,cmd,m_diagram_list.c_str(),m_diagram_list.Length());
		return;
	case Stock_Msg_DepthMarket:
	case Stock_Msg_DiagramData:
	case Stock_Msg_DiagramTag:
	case Stock_Msg_DiagramGroup:
		//E15_Debug::Printf(0,"Stock_Msg_DiagramGroup \n");
		break;
	default:
		return; //纯转发
	}

	const char * ins_id = json->c_str();
	int market = get_market_by_id(ins_id);
	StockData * stock = m_cache->PeekData(market,ins_id );
	if( !stock )
		return ;

	if( !stock->data )
		return;
/*
	m_forward_cnt++;
	if( ( (m_forward_cnt & 0x7ff) == 0 ) )
	{
		E15_Debug::Printf(0,"packet cnt = %lu\n",m_forward_cnt);
		E15_Debug::Log(0,"packet cnt = %lu\n",m_forward_cnt);
	}
*/
	InstrumentInfo * ins = (InstrumentInfo *)stock->data;
	//首先看看是否全部订阅

	//然后看看个性订阅

	do
	{
		if( ins->m_subscribe_hash->Count() > 0 )
			break;

		if( m_subscribe_all->Count() > 0 )
			break;
		return ;
	}while(0);

	forward_param param;
	param.mgr = this;
	param.json = json;
	param.cmd = cmd;
	cmd->type = 0;
	ins->m_subscribe_hash->each( (int (*)(unsigned long ,unsigned long ,unsigned long ,void * ,void * p))do_forward,&param);
	m_subscribe_all->each((int (*)(unsigned long ,unsigned long ,unsigned long ,void * ,void * p))do_forward,&param);

}



