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
#include "stock_data.h"
#include "StockDataCache.h"

#include "ss_util/data_mgr.h"
#include "ss_util/ss_util.h"

#define PRINT_SCREEN	true
#define DEPTH_MARKET_HEAD_LEN 16

#define FIFO_PREFIX	"./tmp_fifo/"
#define DATABASE_IMPORT "./database_import/"

using strategy_export = std::shared_ptr<strategy_base> (*)();
