#ifndef STDAFX_H_
#define STDAFX_H_

//////////////////////////////////////////////////////////////////////////
//standard/system library macro define
#define PRINT_SCREEN true
#define THREAD_NUM 10

#define STRATEGY_NODE "strategy"		//策略服务器节点
#define MARKET_DATA_NODE	"data_factory"		//行情数据节点
#define STRATEGY_PROXY "proxy"			//策略代理节点
#include "crx_pch.h"

//////////////////////////////////////////////////////////////////////////
//e15 library
#include "E15_queue.h"
#include "E15_string.h"
#include "E15_log.h"
#include "E15_zip.h"
#include "E15_value.h"
#include "E15_server.h"
#include "E15_client.h"
#include "E15_console.h"
#include "E15_ini.h"
#include "E15_debug.h"
#include "E15_http_client.h"

//////////////////////////////////////////////////////////////////////////
//history server headers
#include "stock_msg.h"
#include "stock_data.h"
#include "data_trans.h"
#include "StockDataCache.h"
#include "store_history.h"

#include "data_trans.h"
#include "history_mgr.h"

void print_thread_safe(const char *format, ...);

#endif /* STDAFX_H_ */
