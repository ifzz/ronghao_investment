#请修改以下内容以确定相关内容

#项目类型(可执行程序exe,静态库lib,动态库dll,so)
export PROJECT_TYPE=so
export PROJECT_AUTHOR=E15
export PROJECT_VERSION=1.0.0.0

E15_PUBLIC_FILE=

#源文件所在路径 （目录列表）
PROJECT_SOURCE= ../ 

#../../../md/md_data\


#头文件路径(-I目录)
PROJECT_INCLUDE= -I../ \
		 -I/data/runtime/cxx_inc \
		 -I../../include/

#其他的链接路径，(-L目录)
PROJECT_LINK_DIR= -L/data/runtime/linux/so/ -L../../so

#__E15_Socket_epoll
#__E15_Socket_IOCP  //for windows
#__E15_Socket_Select //for all

#工程的宏定义
PROJECT_DEF= -D_LINUX 
export PROJECT_DEF_DEBUG= -std=c++11  -DDEBUG
# -D_DEBUG -DDEBUG 
# release 工程的宏定义
export PROJECT_DEF_RELEASE= -DNDEBUG

#工程需要链接的其他动态库
export PROJECT_IMPORT_DLL=

#-ldl -lc 

# debug 工程需要链接的其他静态库文件列表
export PROJECT_IMPORT_LIB_DEBUG=
#-lE15_cxx_objectD -lE15_stock_analyseD 
export PROJECT_IMPORT_LIB_RELEASE=
#-lE15_cxx_object -lE15_stock_analyse 

#项目编译输出路径
export PROJECT_PATH = ../so/
#项目输出文件主名称(不带后缀）
export PROJECT_BASENAME = strategy_demo
#项目编译中间obj文件路径
export PROJECT_OUT_PATH=/tmp/e15_build/linux/$(PROJECT_BASENAME)/
