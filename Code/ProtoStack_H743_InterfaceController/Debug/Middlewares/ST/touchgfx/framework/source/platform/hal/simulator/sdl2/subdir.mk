################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.cpp \
../Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.cpp \
../Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.cpp 

OBJS += \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.o \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.o \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.o 

CPP_DEPS += \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.d \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.d \
./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/%.o Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/%.su Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/%.cyclo: ../Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/%.cpp Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2

clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2:
	-$(RM) ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.cyclo ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.d ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.o ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.su ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.cyclo ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.d ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.o ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.su ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.cyclo ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.d ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.o ./Middlewares/ST/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.su

.PHONY: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2

