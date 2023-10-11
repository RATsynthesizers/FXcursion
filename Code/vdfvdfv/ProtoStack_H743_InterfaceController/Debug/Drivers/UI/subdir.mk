################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/UI/UARTadapter.c \
../Drivers/UI/ui_adapter_reg.c 

CPP_SRCS += \
../Drivers/UI/Button.cpp \
../Drivers/UI/Encoder.cpp \
../Drivers/UI/Led.cpp \
../Drivers/UI/UIObjecs.cpp 

C_DEPS += \
./Drivers/UI/UARTadapter.d \
./Drivers/UI/ui_adapter_reg.d 

OBJS += \
./Drivers/UI/Button.o \
./Drivers/UI/Encoder.o \
./Drivers/UI/Led.o \
./Drivers/UI/UARTadapter.o \
./Drivers/UI/UIObjecs.o \
./Drivers/UI/ui_adapter_reg.o 

CPP_DEPS += \
./Drivers/UI/Button.d \
./Drivers/UI/Encoder.d \
./Drivers/UI/Led.d \
./Drivers/UI/UIObjecs.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/UI/%.o Drivers/UI/%.su: ../Drivers/UI/%.cpp Drivers/UI/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/UI/%.o Drivers/UI/%.su: ../Drivers/UI/%.c Drivers/UI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-UI

clean-Drivers-2f-UI:
	-$(RM) ./Drivers/UI/Button.d ./Drivers/UI/Button.o ./Drivers/UI/Button.su ./Drivers/UI/Encoder.d ./Drivers/UI/Encoder.o ./Drivers/UI/Encoder.su ./Drivers/UI/Led.d ./Drivers/UI/Led.o ./Drivers/UI/Led.su ./Drivers/UI/UARTadapter.d ./Drivers/UI/UARTadapter.o ./Drivers/UI/UARTadapter.su ./Drivers/UI/UIObjecs.d ./Drivers/UI/UIObjecs.o ./Drivers/UI/UIObjecs.su ./Drivers/UI/ui_adapter_reg.d ./Drivers/UI/ui_adapter_reg.o ./Drivers/UI/ui_adapter_reg.su

.PHONY: clean-Drivers-2f-UI

