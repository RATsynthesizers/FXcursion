################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Modules/libModules/Module.cpp \
../Modules/libModules/Wire.cpp 

OBJS += \
./Modules/libModules/Module.o \
./Modules/libModules/Wire.o 

CPP_DEPS += \
./Modules/libModules/Module.d \
./Modules/libModules/Wire.d 


# Each subdirectory must supply rules for building sources it contributes
Modules/libModules/%.o Modules/libModules/%.su: ../Modules/libModules/%.cpp Modules/libModules/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/Modules" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/DSP_MY" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testAudioTrhru/Drivers/HW" -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Modules-2f-libModules

clean-Modules-2f-libModules:
	-$(RM) ./Modules/libModules/Module.d ./Modules/libModules/Module.o ./Modules/libModules/Module.su ./Modules/libModules/Wire.d ./Modules/libModules/Wire.o ./Modules/libModules/Wire.su

.PHONY: clean-Modules-2f-libModules

