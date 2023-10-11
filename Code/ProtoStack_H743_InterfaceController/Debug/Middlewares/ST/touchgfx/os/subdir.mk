################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Middlewares/ST/touchgfx/os/OSWrappers.cpp \
../Middlewares/ST/touchgfx/os/OSWrappers_cmsis.cpp 

OBJS += \
./Middlewares/ST/touchgfx/os/OSWrappers.o \
./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.o 

CPP_DEPS += \
./Middlewares/ST/touchgfx/os/OSWrappers.d \
./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/touchgfx/os/%.o Middlewares/ST/touchgfx/os/%.su Middlewares/ST/touchgfx/os/%.cyclo: ../Middlewares/ST/touchgfx/os/%.cpp Middlewares/ST/touchgfx/os/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-touchgfx-2f-os

clean-Middlewares-2f-ST-2f-touchgfx-2f-os:
	-$(RM) ./Middlewares/ST/touchgfx/os/OSWrappers.cyclo ./Middlewares/ST/touchgfx/os/OSWrappers.d ./Middlewares/ST/touchgfx/os/OSWrappers.o ./Middlewares/ST/touchgfx/os/OSWrappers.su ./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.cyclo ./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.d ./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.o ./Middlewares/ST/touchgfx/os/OSWrappers_cmsis.su

.PHONY: clean-Middlewares-2f-ST-2f-touchgfx-2f-os

