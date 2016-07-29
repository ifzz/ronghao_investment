#请修改以下内容以确定相关内容

#项目类型(可执行程序exe,静态库lib,动态库dll,so)
export PROJECT_TYPE=exe
export PROJECT_AUTHOR=E15
export PROJECT_VERSION=1.0.0.0

-include gtk.mk

E15_PUBLIC_FILE=

#源文件所在路径 （目录列表）
PROJECT_SOURCE= ../gtk ../../market

#头文件路径(-I目录)
PROJECT_INCLUDE= -I/data/runtime/cxx_inc \
		 -I../gtk \
		 -I../../market \


#其他的链接路径，(-L目录)
PROJECT_LINK_DIR= -L/data/runtime/win32 \
		  -L/data/runtime/ctp/win32 \
		  -L/data/runtime/gtk30/mingw32/lib \


#工程的宏定义
PROJECT_DEF= -DWIN32 $(GTK_CFLAG)

# debug 工程的宏定义
export PROJECT_DEF_DEBUG= -D_DEBUG -DDEBUG 

# release 工程的宏定义
export PROJECT_DEF_RELEASE= -DNDEBUG

#工程需要链接的其他动态库
export PROJECT_IMPORT_DLL= $(GTK_LIBS) -lE15_cxx_objectD 

# debug 工程需要链接的其他静态库文件列表
export PROJECT_IMPORT_LIB_DEBUG=
export PROJECT_IMPORT_LIB_RELEASE=

#项目编译输出路径
export PROJECT_PATH = /data/runtime/win32/

#项目输出文件主名称(不带后缀）

export PROJECT_BASENAME = radar
export PROJECT_OUT_PATH=/tmp/e15_build/win32/$(PROJECT_BASENAME)


