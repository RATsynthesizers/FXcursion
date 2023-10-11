################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Modules/Effects/phaser/Phaser.cpp 

OBJS += \
./Modules/Effects/phaser/Phaser.o 

CPP_DEPS += \
./Modules/Effects/phaser/Phaser.d 


# Each subdirectory must supply rules for building sources it contributes
Modules/Effects/phaser/%.o Modules/Effects/phaser/%.su: ../Modules/Effects/phaser/%.cpp Modules/Effects/phaser/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/vdfvdfv/ProtoStack_H743_testAudioTrhru/Modules" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/vdfvdfv/ProtoStack_H743_testAudioTrhru/DSP_MY" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/vdfvdfv/ProtoStack_H743_testAudioTrhru/Drivers/HW" -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Modules-2f-Effects-2f-phaser

clean-Modules-2f-Effects-2f-phaser:
	-$(RM) ./Modules/Effects/phaser/Phaser.d ./Modules/Effects/phaser/Phaser.o ./Modules/Effects/phaser/Phaser.su

.PHONY: clean-Modules-2f-Effects-2f-phaser

