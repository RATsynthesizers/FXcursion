################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Modules/amp/Amp.cpp 

OBJS += \
./Modules/amp/Amp.o 

CPP_DEPS += \
./Modules/amp/Amp.d 


# Each subdirectory must supply rules for building sources it contributes
Modules/amp/%.o Modules/amp/%.su: ../Modules/amp/%.cpp Modules/amp/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/Modules" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/DSP_MY" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/Drivers/HW" -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Modules-2f-amp

clean-Modules-2f-amp:
	-$(RM) ./Modules/amp/Amp.d ./Modules/amp/Amp.o ./Modules/amp/Amp.su

.PHONY: clean-Modules-2f-amp

