#!/bin/bash

runtime=/data/runtime/linux

# 动态库的路径
export LD_LIBRARY_PATH=$runtime/so/:$runtime/ctp:$runtime/hh_api

# 6、A股行情
#cd $runtime/stock/hh_server/
#./hh_serverD.exe -stop

# 5、期货行情
cd $runtime/stock/ctp_server/
./ctp_serverD.exe -stop

# 4、历史数据服务器
cd $runtime/stock/history_server/
./historyD.exe -stop

# 3、客户端数据转发服务器
cd $runtime/stock/ClientMgr/
./ClientMgrD.exe -stop

# 2、数据加工服务器
cd $runtime/stock/DiagramServer/
./DiagramSerD.exe -stop

# 1、站点服务器
cd $runtime/application/domain/
./E15_domainD.exe -stop




