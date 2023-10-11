################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/MyTouchGFXadapter/GFXadapter.c 

CPP_SRCS += \
../Drivers/MyTouchGFXadapter/MyButtonController.cpp 

C_DEPS += \
./Drivers/MyTouchGFXadapter/GFXadapter.d 

OBJS += \
./Drivers/MyTouchGFXadapter/GFXadapter.o \
./Drivers/MyTouchGFXadapter/MyButtonController.o 

CPP_DEPS += \
./Drivers/MyTouchGFXadapter/MyButtonController.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/MyTouchGFXadapter/%.o Drivers/MyTouchGFXadapter/%.su: ../Drivers/MyTouchGFXadapter/%.c Drivers/MyTouchGFXadapter/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/MyTouchGFXadapter/%.o Drivers/MyTouchGFXadapter/%.su: ../Drivers/MyTouchGFXadapter/%.cpp Drivers/MyTouchGFXadapter/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-MyTouchGFXadapter

clean-Drivers-2f-MyTouchGFXadapter:
	-$(RM) ./Drivers/MyTouchGFXadapter/GFXadapter.d ./Drivers/MyTouchGFXadapter/GFXadapter.o ./Drivers/MyTouchGFXadapter/GFXadapter.su ./Drivers/MyTouchGFXadapter/MyButtonController.d ./Drivers/MyTouchGFXadapter/MyButtonController.o ./Drivers/MyTouchGFXadapter/MyButtonController.su

.PHONY: clean-Drivers-2f-MyTouchGFXadapter

