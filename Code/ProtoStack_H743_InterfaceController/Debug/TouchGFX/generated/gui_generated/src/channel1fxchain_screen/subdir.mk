################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.cpp 

OBJS += \
./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.o 

CPP_DEPS += \
./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/gui_generated/src/channel1fxchain_screen/%.o TouchGFX/generated/gui_generated/src/channel1fxchain_screen/%.su TouchGFX/generated/gui_generated/src/channel1fxchain_screen/%.cyclo: ../TouchGFX/generated/gui_generated/src/channel1fxchain_screen/%.cpp TouchGFX/generated/gui_generated/src/channel1fxchain_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-channel1fxchain_screen

clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-channel1fxchain_screen:
	-$(RM) ./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.cyclo ./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.d ./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.o ./TouchGFX/generated/gui_generated/src/channel1fxchain_screen/Channel1FXchainViewBase.su

.PHONY: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-channel1fxchain_screen

