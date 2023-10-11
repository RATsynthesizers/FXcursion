################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/HW/i2c_lcd.c \
../Drivers/HW/ly68l6400.c 

OBJS += \
./Drivers/HW/i2c_lcd.o \
./Drivers/HW/ly68l6400.o 

C_DEPS += \
./Drivers/HW/i2c_lcd.d \
./Drivers/HW/ly68l6400.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/HW/%.o Drivers/HW/%.su: ../Drivers/HW/%.c Drivers/HW/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I"D:/YandexDisk/_PEDAL_BreadBoard/Code/Emptiness/Drivers/HW" -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-HW

clean-Drivers-2f-HW:
	-$(RM) ./Drivers/HW/i2c_lcd.d ./Drivers/HW/i2c_lcd.o ./Drivers/HW/i2c_lcd.su ./Drivers/HW/ly68l6400.d ./Drivers/HW/ly68l6400.o ./Drivers/HW/ly68l6400.su

.PHONY: clean-Drivers-2f-HW

