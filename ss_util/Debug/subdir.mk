################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../DiagramDataFactory.cpp \
../data_mgr.cpp \
../ss_util.cpp 

OBJS += \
./DiagramDataFactory.o \
./data_mgr.o \
./ss_util.o 

CPP_DEPS += \
./DiagramDataFactory.d \
./data_mgr.d \
./ss_util.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -fPIC -I/home/daniel/workspace/ronghao_investment/include -I/home/daniel/workspace/ronghao_investment/ver2/sdk/include -I/home/daniel/workspace/ronghao_investment/ver2/cxx_inc -I/home/daniel/workspace/ronghao_investment/ver2/dev/stock_inc -I/home/daniel/workspace/common_library/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


