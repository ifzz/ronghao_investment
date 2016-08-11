################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../strategy_proxy.cpp 

OBJS += \
./strategy_proxy.o 

CPP_DEPS += \
./strategy_proxy.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I/home/daniel/workspace/ronghao_investment/ver2/cxx_inc -I/home/daniel/workspace/ronghao_investment/ver2/dev/stock_inc -I/home/daniel/workspace/common_library/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


