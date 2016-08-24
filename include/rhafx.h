#pragma once

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

#include "crx_pch.h"
#include "strategy_base.h"
#include "stock_msg.h"
#include "store_history.h"
#include "stock_data.h"
#include "StockDataCache.h"

#include "ss_util/data_mgr.h"
#include "ss_util/ss_util.h"

struct stg_config {
	std::string ip;
	int port;
	std::string user;
	std::string passwd;

	int sub_all;
	int conn_real;
	int for_produce;

	int threads_num;
	std::string stg_dir;
	std::string db_import;

	std::string md_role;			//行情数据节点 "proxy"
	std::string his_md_role;		//历史行情节点
	std::string trade_role;			//交易服务器节点  "trade"
	std::string cli_prx_role;		//前端界面代理节点

	std::string so_addr;			//so文件服务器地址
	int so_port;						//端口
};

#define DEPTH_MARKET_HEAD_LEN 16
#define FIFO_PREFIX	"./runtime_fifo/"

using strategy_export = std::shared_ptr<strategy_base> (*)();
