################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/HW/LY68L6400.c \
../Drivers/HW/MySAIadapter.c \
../Drivers/HW/wm8731.c 

C_DEPS += \
./Drivers/HW/LY68L6400.d \
./Drivers/HW/MySAIadapter.d \
./Drivers/HW/wm8731.d 

OBJS += \
./Drivers/HW/LY68L6400.o \
./Drivers/HW/MySAIadapter.o \
./Drivers/HW/wm8731.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/HW/%.o Drivers/HW/%.su: ../Drivers/HW/%.c Drivers/HW/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -DARM_MATH_CM7 -D__FPU_PRESENT=1 -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/HW -I../Modules -I../Modules/libModules -I../Modules/amp -I../Modules/reverb -I../DSP_MY -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-HW

clean-Drivers-2f-HW:
	-$(RM) ./Drivers/HW/LY68L6400.d ./Drivers/HW/LY68L6400.o ./Drivers/HW/LY68L6400.su ./Drivers/HW/MySAIadapter.d ./Drivers/HW/MySAIadapter.o ./Drivers/HW/MySAIadapter.su ./Drivers/HW/wm8731.d ./Drivers/HW/wm8731.o ./Drivers/HW/wm8731.su

.PHONY: clean-Drivers-2f-HW

