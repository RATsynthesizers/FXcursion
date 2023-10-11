################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/HW/GUIadapter.c \
../Drivers/HW/MySAIadapter.c \
../Drivers/HW/MyTDMadapter.c \
../Drivers/HW/cs42448.c \
../Drivers/HW/wm8731.c 

C_DEPS += \
./Drivers/HW/GUIadapter.d \
./Drivers/HW/MySAIadapter.d \
./Drivers/HW/MyTDMadapter.d \
./Drivers/HW/cs42448.d \
./Drivers/HW/wm8731.d 

OBJS += \
./Drivers/HW/GUIadapter.o \
./Drivers/HW/MySAIadapter.o \
./Drivers/HW/MyTDMadapter.o \
./Drivers/HW/cs42448.o \
./Drivers/HW/wm8731.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/HW/%.o Drivers/HW/%.su: ../Drivers/HW/%.c Drivers/HW/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/Modules" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/DSP_MY" -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/ProtoStack_H743_testTDMTrhru/Drivers/HW" -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-HW

clean-Drivers-2f-HW:
	-$(RM) ./Drivers/HW/GUIadapter.d ./Drivers/HW/GUIadapter.o ./Drivers/HW/GUIadapter.su ./Drivers/HW/MySAIadapter.d ./Drivers/HW/MySAIadapter.o ./Drivers/HW/MySAIadapter.su ./Drivers/HW/MyTDMadapter.d ./Drivers/HW/MyTDMadapter.o ./Drivers/HW/MyTDMadapter.su ./Drivers/HW/cs42448.d ./Drivers/HW/cs42448.o ./Drivers/HW/cs42448.su ./Drivers/HW/wm8731.d ./Drivers/HW/wm8731.o ./Drivers/HW/wm8731.su

.PHONY: clean-Drivers-2f-HW

