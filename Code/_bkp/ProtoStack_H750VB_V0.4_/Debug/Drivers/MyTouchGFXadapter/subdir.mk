################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/MyTouchGFXadapter/GFXadapter.c 

C_DEPS += \
./Drivers/MyTouchGFXadapter/GFXadapter.d 

OBJS += \
./Drivers/MyTouchGFXadapter/GFXadapter.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/MyTouchGFXadapter/%.o Drivers/MyTouchGFXadapter/%.su: ../Drivers/MyTouchGFXadapter/%.c Drivers/MyTouchGFXadapter/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H750xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/HW -I../Modules -I../Modules/libModules -I../Modules/amp -I../Modules/reverb -I../DSP_MY -I../Drivers/MyTouchGFXadapter -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-MyTouchGFXadapter

clean-Drivers-2f-MyTouchGFXadapter:
	-$(RM) ./Drivers/MyTouchGFXadapter/GFXadapter.d ./Drivers/MyTouchGFXadapter/GFXadapter.o ./Drivers/MyTouchGFXadapter/GFXadapter.su

.PHONY: clean-Drivers-2f-MyTouchGFXadapter

