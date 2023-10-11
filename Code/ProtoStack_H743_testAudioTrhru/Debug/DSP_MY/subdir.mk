################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../DSP_MY/dsp.cpp 

OBJS += \
./DSP_MY/dsp.o 

CPP_DEPS += \
./DSP_MY/dsp.d 


# Each subdirectory must supply rules for building sources it contributes
DSP_MY/%.o DSP_MY/%.su: ../DSP_MY/%.cpp DSP_MY/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/Modules" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/DSP_MY" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/Drivers/HW" -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-DSP_MY

clean-DSP_MY:
	-$(RM) ./DSP_MY/dsp.d ./DSP_MY/dsp.o ./DSP_MY/dsp.su

.PHONY: clean-DSP_MY

