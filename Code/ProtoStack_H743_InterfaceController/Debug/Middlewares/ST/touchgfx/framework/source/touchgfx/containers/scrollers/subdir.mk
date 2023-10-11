################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.cpp 

OBJS += \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.o 

CPP_DEPS += \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/%.o Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/%.su Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/%.cyclo: ../Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/%.cpp Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers-2f-scrollers

clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers-2f-scrollers:
	-$(RM) ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/DrawableList.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollBase.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollList.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheel.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelBase.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/containers/scrollers/ScrollWheelWithSelectionStyle.su

.PHONY: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers-2f-scrollers

