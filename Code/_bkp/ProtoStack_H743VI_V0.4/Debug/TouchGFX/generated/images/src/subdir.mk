################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/images/src/BitmapDatabase.cpp \
../TouchGFX/generated/images/src/image_GaugeExt.cpp \
../TouchGFX/generated/images/src/image_GaugeNew.cpp \
../TouchGFX/generated/images/src/image_GaugePointer.cpp \
../TouchGFX/generated/images/src/image_GaugePointerNew.cpp \
../TouchGFX/generated/images/src/image_g473.cpp 

OBJS += \
./TouchGFX/generated/images/src/BitmapDatabase.o \
./TouchGFX/generated/images/src/image_GaugeExt.o \
./TouchGFX/generated/images/src/image_GaugeNew.o \
./TouchGFX/generated/images/src/image_GaugePointer.o \
./TouchGFX/generated/images/src/image_GaugePointerNew.o \
./TouchGFX/generated/images/src/image_g473.o 

CPP_DEPS += \
./TouchGFX/generated/images/src/BitmapDatabase.d \
./TouchGFX/generated/images/src/image_GaugeExt.d \
./TouchGFX/generated/images/src/image_GaugeNew.d \
./TouchGFX/generated/images/src/image_GaugePointer.d \
./TouchGFX/generated/images/src/image_GaugePointerNew.d \
./TouchGFX/generated/images/src/image_g473.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/images/src/%.o TouchGFX/generated/images/src/%.su: ../TouchGFX/generated/images/src/%.cpp TouchGFX/generated/images/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -Og -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-images-2f-src

clean-TouchGFX-2f-generated-2f-images-2f-src:
	-$(RM) ./TouchGFX/generated/images/src/BitmapDatabase.d ./TouchGFX/generated/images/src/BitmapDatabase.o ./TouchGFX/generated/images/src/BitmapDatabase.su ./TouchGFX/generated/images/src/image_GaugeExt.d ./TouchGFX/generated/images/src/image_GaugeExt.o ./TouchGFX/generated/images/src/image_GaugeExt.su ./TouchGFX/generated/images/src/image_GaugeNew.d ./TouchGFX/generated/images/src/image_GaugeNew.o ./TouchGFX/generated/images/src/image_GaugeNew.su ./TouchGFX/generated/images/src/image_GaugePointer.d ./TouchGFX/generated/images/src/image_GaugePointer.o ./TouchGFX/generated/images/src/image_GaugePointer.su ./TouchGFX/generated/images/src/image_GaugePointerNew.d ./TouchGFX/generated/images/src/image_GaugePointerNew.o ./TouchGFX/generated/images/src/image_GaugePointerNew.su ./TouchGFX/generated/images/src/image_g473.d ./TouchGFX/generated/images/src/image_g473.o ./TouchGFX/generated/images/src/image_g473.su

.PHONY: clean-TouchGFX-2f-generated-2f-images-2f-src

