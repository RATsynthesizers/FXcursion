################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/HW/ssd1305_i2c/ssd1305_i2c.c 

C_DEPS += \
./Drivers/HW/ssd1305_i2c/ssd1305_i2c.d 

OBJS += \
./Drivers/HW/ssd1305_i2c/ssd1305_i2c.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/HW/ssd1305_i2c/%.o Drivers/HW/ssd1305_i2c/%.su: ../Drivers/HW/ssd1305_i2c/%.c Drivers/HW/ssd1305_i2c/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -DARM_MATH_CM7 -D__FPU_PRESENT=1 -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/HW -I../Modules -I../Modules/libModules -I../Modules/amp -I../Modules/reverb -I../DSP_MY -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-HW-2f-ssd1305_i2c

clean-Drivers-2f-HW-2f-ssd1305_i2c:
	-$(RM) ./Drivers/HW/ssd1305_i2c/ssd1305_i2c.d ./Drivers/HW/ssd1305_i2c/ssd1305_i2c.o ./Drivers/HW/ssd1305_i2c/ssd1305_i2c.su

.PHONY: clean-Drivers-2f-HW-2f-ssd1305_i2c

