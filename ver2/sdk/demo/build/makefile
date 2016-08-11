-include project.mk
-include cross_compiler.mk

######################################################################
#--------------以下内容不需要修改------------------------------------#
######################################################################

export PROJECT_CC= $(BUILD_TOOLS_DIR)$(BUILD_PREFIX)gcc
export PROJECT_CXX= $(BUILD_TOOLS_DIR)$(BUILD_PREFIX)g++
export PROJECT_LINKER= $(BUILD_TOOLS_DIR)$(BUILD_PREFIX)g++
export PROJECT_AR= $(BUILD_TOOLS_DIR)$(BUILD_PREFIX)ar rv

BUILD_T = `date '+%Y-%m-%d_%H:%M:%S'`
export PROJECT_DBUILD_TIME=$(BUILD_T)

CXX__DLL_FLAG=
CC_DLL_FLAG=
ifeq ($(PROJECT_TYPE),so)
CXX_DLL_FLAG= -fPIC 
#-fvisibility=hidden -fvisibility-inlines-hidden
CC_DLL_FLAG= -fPIC 
#-fvisibility=hidden
endif

ifeq ($(PROJECT_TYPE),dll)
CXX_DLL_FLAG= -fPIC 
#-fvisibility=hidden -fvisibility-inlines-hidden
CC_DLL_FLAG= -fPIC 
#-fvisibility=hidden
endif

ifeq ($(PROJECT_TYPE),lib)
CXX_DLL_FLAG= -D_LIB
CC_DLL_FLAG= -D_LIB
endif

#编译用的编译器参数
export PROJECT_CXX_FLAGS = $(PROJECT_BIT) $(PROJECT_INCLUDE) $(CXX_DLL_FLAG) $(SYSROOT_DIR) -Wall $(PROJECT_DEF)  \
	 -DE15_PROJECT_AUTHOR=\"$(PROJECT_AUTHOR)\" -DE15_PROJECT_VERSION=\"$(PROJECT_VERSION)\" -DE15_PROJECT_BUILD_TIME=\"$(PROJECT_DBUILD_TIME)\"
export PROJECT_CC_FLAGS = $(PROJECT_BIT) $(PROJECT_INCLUDE) $(CC_DLL_FLAG) $(SYSROOT_DIR) -Wall $(PROJECT_DEF)  \
	-DE15_PROJECT_AUTHOR=\"$(PROJECT_AUTHOR)\" -DE15_PROJECT_VERSION=\"$(PROJECT_VERSION)\" -DE15_PROJECT_BUILD_TIME=\"$(PROJECT_DBUILD_TIME)\"

#链接用的参数
export PROJECT_EXE_LINK_FLAGS =$(PROJECT_BIT) $(PROJECT_LINK_DIR) $(SYSROOT_DIR) \
	-Wl,-rpath,./ -Wl,--unresolved-symbols=ignore-in-shared-libs
export PROJECT_DLL_LINK_FLAGS = $(PROJECT_BIT) $(PROJECT_LINK_DIR) $(SYSROOT_DIR) \
	-rdynamic -shared  -lpthread  

#-Wl,--unresolved-symbols=ignore-in-shared-libs

#PROJECT源文件列表 
PROJECT_CPP_FILES :=$(foreach dir,$(PROJECT_SOURCE),$(wildcard $(dir)/*.cpp))
PROJECT_CXX_FILES :=$(foreach dir,$(PROJECT_SOURCE),$(wildcard $(dir)/*.cxx))


PROJECT_C_FILES :=$(foreach dir,$(PROJECT_SOURCE),$(wildcard $(dir)/*.c))

ifeq ($(PROJECT_TYPE),so)
PROJECT_C_FILES :=$(foreach dir,$(PROJECT_SOURCE),$(wildcard $(dir)/*.c)) 
#./E15_dll_main.c
endif

ifeq ($(PROJECT_TYPE),dll)
PROJECT_C_FILES :=$(foreach dir,$(PROJECT_SOURCE),$(wildcard $(dir)/*.c)) 
#./E15_dll_main.c
endif

PROJECT_CPP_DEP := $(PROJECT_CPP_FILES:%.cpp=%.d)
PROJECT_CXX_DEP := $(PROJECT_CXX_FILES:%.cxx=%.d)
PROJECT_C_DEP := $(PROJECT_C_FILES:%.c=%.d)

export PROJECT_SRC_FILES :=$(PROJECT_CPP_FILES) $(PROJECT_CXX_FILES) $(PROJECT_C_FILES) 
export PROJECT_DEPS := $(PROJECT_CPP_DEP) $(PROJECT_CXX_DEP) $(PROJECT_C_DEP)
export PROJECT_OBJS := $(PROJECT_DEPS:%.d=%.o)

#分析版本信息
empty:=
space:= $(empty) $(empty)
PROJECT_VERSION_TEMP:= $(subst .,$(space),$(PROJECT_VERSION))
export PROJECT_VERSION_MAJOR = $(word 1,$(PROJECT_VERSION_TEMP))
export PROJECT_VERSION_MINOR = $(word 2,$(PROJECT_VERSION_TEMP) )

default: debug

all: debug release

.PHONY :debug
debug: 
	@echo $(PROJECT_DBUILD_TIME)
	@-mkdir -p $(PROJECT_OUT_PATH)./debug
	@-mkdir -p $(PROJECT_PATH)
	@-make -f makefile.debug

.PHONY: release 
release: 
	@-mkdir -p $(PROJECT_OUT_PATH)./release
	@-mkdir -p $(PROJECT_PATH)
	@-make -f makefile.release

.PHONY :clean
clean: clean-d clean-r

.PHONY :clean-d
clean-d:
	@-make -f makefile.debug clean
	@-rm -rf $(PROJECT_OUT_PATH).debug

.PHONY :clean-r
clean-r:
	@-make -f makefile.release clean
	@-rm -rf $(PROJECT_OUT_PATH).release
	
rebuild: clean all

