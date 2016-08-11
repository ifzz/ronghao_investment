################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pre-build main-build

# Main-build Target
main-build: ../../runtime/strategy_server/strategy_server

# Tool invocations
../../runtime/strategy_server/strategy_server: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -Wl,-rpath=. -Wl,-rpath=../so -Wl,-rpath=/home/daniel/workspace/ronghao_investment/ver2/runtime/so -rdynamic -L/home/daniel/workspace/ronghao_investment/ver2/runtime/so -L/home/daniel/workspace/ronghao_investment/runtime/so -o "../../runtime/strategy_server/strategy_server" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) ../../runtime/strategy_server/strategy_server
	-@echo ' '

pre-build:
	-mkdir -p ../../runtime/strategy_server
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: main-build pre-build

-include ../makefile.targets
