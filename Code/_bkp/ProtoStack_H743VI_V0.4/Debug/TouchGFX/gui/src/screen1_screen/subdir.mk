################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/screen1_screen/Screen1Presenter.cpp \
../TouchGFX/gui/src/screen1_screen/Screen1View.cpp 

OBJS += \
./TouchGFX/gui/src/screen1_screen/Screen1Presenter.o \
./TouchGFX/gui/src/screen1_screen/Screen1View.o 

CPP_DEPS += \
./TouchGFX/gui/src/screen1_screen/Screen1Presenter.d \
./TouchGFX/gui/src/screen1_screen/Screen1View.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/screen1_screen/%.o TouchGFX/gui/src/screen1_screen/%.su: ../TouchGFX/gui/src/screen1_screen/%.cpp TouchGFX/gui/src/screen1_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -Og -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-screen1_screen

clean-TouchGFX-2f-gui-2f-src-2f-screen1_screen:
	-$(RM) ./TouchGFX/gui/src/screen1_screen/Screen1Presenter.d ./TouchGFX/gui/src/screen1_screen/Screen1Presenter.o ./TouchGFX/gui/src/screen1_screen/Screen1Presenter.su ./TouchGFX/gui/src/screen1_screen/Screen1View.d ./TouchGFX/gui/src/screen1_screen/Screen1View.o ./TouchGFX/gui/src/screen1_screen/Screen1View.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-screen1_screen

