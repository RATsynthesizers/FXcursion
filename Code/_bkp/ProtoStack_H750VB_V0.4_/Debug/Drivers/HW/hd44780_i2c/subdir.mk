################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/HW/hd44780_i2c/hd44780_i2c.cpp 

OBJS += \
./Drivers/HW/hd44780_i2c/hd44780_i2c.o 

CPP_DEPS += \
./Drivers/HW/hd44780_i2c/hd44780_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/HW/hd44780_i2c/%.o Drivers/HW/hd44780_i2c/%.su: ../Drivers/HW/hd44780_i2c/%.cpp Drivers/HW/hd44780_i2c/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -DARM_MATH_CM7 -D__FPU_PRESENT=1 -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/MyTouchGFXadapter -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-HW-2f-hd44780_i2c

clean-Drivers-2f-HW-2f-hd44780_i2c:
	-$(RM) ./Drivers/HW/hd44780_i2c/hd44780_i2c.d ./Drivers/HW/hd44780_i2c/hd44780_i2c.o ./Drivers/HW/hd44780_i2c/hd44780_i2c.su

.PHONY: clean-Drivers-2f-HW-2f-hd44780_i2c
