#ifndef __Client_Mgr_H
#define __Client_Mgr_H

#include "E15_server.h"
#include "E15_value.h"

#include "E15_map.h"
#include "E15_queue.h"
#include "E15_zip.h"

class StockDataCache;

class Client_Mgr : public E15_Server
{
public:
	Client_Mgr();
	virtual ~Client_Mgr();

	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json) ;
	virtual int OnClose(E15_ServerInfo * info) ;
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) ;
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);


public:

	int  HandleSubscribeById(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) ;
	void HandleSubscribeAll(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *&  json);


private:

	E15_Intmap 			* m_subscribe_all; 		//订阅全部数据的client
  StockDataCache 	* m_cache;				//查找订阅关系

	E15_ValueTable 	m_notify;
	E15_ValueTable  m_vt;

	E15_ValueTable	m_instrument_list;
	E15_String 		  m_instrument_list_buffer;
	E15_String			m_diagram_info;
	E15_String			m_diagram_list;

	//E15_String		  m_
	unsigned long m_forward_cnt;

	E15_Zip					m_zip;
};

extern Client_Mgr * g_server;


#endif
