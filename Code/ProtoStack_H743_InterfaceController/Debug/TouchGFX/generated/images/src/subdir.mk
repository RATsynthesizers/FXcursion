################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/images/src/BitmapDatabase.cpp \
../TouchGFX/generated/images/src/SVGDatabase.cpp \
../TouchGFX/generated/images/src/image_emptyPict.cpp \
../TouchGFX/generated/images/src/image_emptySelectedPict.cpp \
../TouchGFX/generated/images/src/image_empty_dashed.cpp \
../TouchGFX/generated/images/src/image_empty_dashed_selected.cpp \
../TouchGFX/generated/images/src/image_looper.cpp \
../TouchGFX/generated/images/src/image_looper_selected.cpp \
../TouchGFX/generated/images/src/image_rat_logo.cpp 

OBJS += \
./TouchGFX/generated/images/src/BitmapDatabase.o \
./TouchGFX/generated/images/src/SVGDatabase.o \
./TouchGFX/generated/images/src/image_emptyPict.o \
./TouchGFX/generated/images/src/image_emptySelectedPict.o \
./TouchGFX/generated/images/src/image_empty_dashed.o \
./TouchGFX/generated/images/src/image_empty_dashed_selected.o \
./TouchGFX/generated/images/src/image_looper.o \
./TouchGFX/generated/images/src/image_looper_selected.o \
./TouchGFX/generated/images/src/image_rat_logo.o 

CPP_DEPS += \
./TouchGFX/generated/images/src/BitmapDatabase.d \
./TouchGFX/generated/images/src/SVGDatabase.d \
./TouchGFX/generated/images/src/image_emptyPict.d \
./TouchGFX/generated/images/src/image_emptySelectedPict.d \
./TouchGFX/generated/images/src/image_empty_dashed.d \
./TouchGFX/generated/images/src/image_empty_dashed_selected.d \
./TouchGFX/generated/images/src/image_looper.d \
./TouchGFX/generated/images/src/image_looper_selected.d \
./TouchGFX/generated/images/src/image_rat_logo.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/images/src/%.o TouchGFX/generated/images/src/%.su: ../TouchGFX/generated/images/src/%.cpp TouchGFX/generated/images/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-images-2f-src

clean-TouchGFX-2f-generated-2f-images-2f-src:
	-$(RM) ./TouchGFX/generated/images/src/BitmapDatabase.d ./TouchGFX/generated/images/src/BitmapDatabase.o ./TouchGFX/generated/images/src/BitmapDatabase.su ./TouchGFX/generated/images/src/SVGDatabase.d ./TouchGFX/generated/images/src/SVGDatabase.o ./TouchGFX/generated/images/src/SVGDatabase.su ./TouchGFX/generated/images/src/image_emptyPict.d ./TouchGFX/generated/images/src/image_emptyPict.o ./TouchGFX/generated/images/src/image_emptyPict.su ./TouchGFX/generated/images/src/image_emptySelectedPict.d ./TouchGFX/generated/images/src/image_emptySelectedPict.o ./TouchGFX/generated/images/src/image_emptySelectedPict.su ./TouchGFX/generated/images/src/image_empty_dashed.d ./TouchGFX/generated/images/src/image_empty_dashed.o ./TouchGFX/generated/images/src/image_empty_dashed.su ./TouchGFX/generated/images/src/image_empty_dashed_selected.d ./TouchGFX/generated/images/src/image_empty_dashed_selected.o ./TouchGFX/generated/images/src/image_empty_dashed_selected.su ./TouchGFX/generated/images/src/image_looper.d ./TouchGFX/generated/images/src/image_looper.o ./TouchGFX/generated/images/src/image_looper.su ./TouchGFX/generated/images/src/image_looper_selected.d ./TouchGFX/generated/images/src/image_looper_selected.o ./TouchGFX/generated/images/src/image_looper_selected.su ./TouchGFX/generated/images/src/image_rat_logo.d ./TouchGFX/generated/images/src/image_rat_logo.o ./TouchGFX/generated/images/src/image_rat_logo.su

.PHONY: clean-TouchGFX-2f-generated-2f-images-2f-src

