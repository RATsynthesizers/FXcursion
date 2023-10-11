################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../TouchGFX/App/app_touchgfx.c 

C_DEPS += \
./TouchGFX/App/app_touchgfx.d 

OBJS += \
./TouchGFX/App/app_touchgfx.o 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/App/%.o TouchGFX/App/%.su: ../TouchGFX/App/%.c TouchGFX/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-App

clean-TouchGFX-2f-App:
	-$(RM) ./TouchGFX/App/app_touchgfx.d ./TouchGFX/App/app_touchgfx.o ./TouchGFX/App/app_touchgfx.su

.PHONY: clean-TouchGFX-2f-App

