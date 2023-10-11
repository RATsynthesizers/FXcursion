################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ili9341/ili9341.c 

C_DEPS += \
./Drivers/ili9341/ili9341.d 

OBJS += \
./Drivers/ili9341/ili9341.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ili9341/%.o Drivers/ili9341/%.su: ../Drivers/ili9341/%.c Drivers/ili9341/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ili9341

clean-Drivers-2f-ili9341:
	-$(RM) ./Drivers/ili9341/ili9341.d ./Drivers/ili9341/ili9341.o ./Drivers/ili9341/ili9341.su

.PHONY: clean-Drivers-2f-ili9341
