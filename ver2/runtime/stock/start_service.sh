#!/bin/bash

runtime=/data/runtime/linux

# 动态库的路径
export LD_LIBRARY_PATH=$runtime/so/:$runtime/ctp:$runtime/hh_api

# 1、站点服务器
cd $runtime/application/domain/
./E15_domainD.exe -start


# 2、数据加工服务器
cd $runtime/stock/DiagramServer/
./DiagramSerD.exe -start


# 3、客户端数据转发服务器
cd $runtime/stock/ClientMgr/
./ClientMgrD.exe -start


# 4、历史数据服务器
cd $runtime/stock/history_server/
./historyD.exe -start


# 5、期货行情
cd $runtime/stock/ctp_server/
./ctp_serverD.exe -start

# 6、A股行情
#cd $runtime/stock/hh_server/
#./hh_serverD.exe -start

