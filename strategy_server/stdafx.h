#ifndef STDAFX_H_
#define STDAFX_H_

//////////////////////////////////////////////////////////////////////////
//standard/system library macro define
#define MARKET_DATA_NODE	"proxy"		//行情数据节点
#define TRADE_SERVER_NODE "trade"		//交易服务器节点

//该服务器会和运维和前端发生通信
#define CLIENT_UI_NODE "XXX"				//前端界面节点
#define OPE_MANAGER_NODE "YYY"		//后台运维管理节点

//so文件服务器地址
#define FILE_SERVER_IP			"127.0.0.1"
#define FILE_SERVER_PORT		10000

#define TEST_INTERVAL 1000		//1s作一次统计
#define THREADS_NUM 20

#define FIFO_PREFIX	"./tmp_fifo/"
#define STRATEGY_DIR "./strategy_so/"
#define DATABASE_IMPORT "./database_import/"

#include "rhafx.h"
#include "data_manager.h"
#include "strategy_manager.h"

struct config {
	std::string ip;
	int port;
	std::string user;
	std::string passwd;

	int conn_real;
	int for_produce;
};

extern E15_Log g_log;
extern E15_Socket g_socket;
extern config g_conf;

#endif		//STDAFX_H_
