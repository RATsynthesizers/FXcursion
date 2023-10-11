################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/MenuItemBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.cpp 

OBJS += \
./TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.o \
./TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.o \
./TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.o \
./TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.o \
./TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.o \
./TouchGFX/generated/gui_generated/src/containers/MenuItemBase.o \
./TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.o \
./TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.o \
./TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.o \
./TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.o \
./TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.o \
./TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.o 

CPP_DEPS += \
./TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.d \
./TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.d \
./TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.d \
./TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.d \
./TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.d \
./TouchGFX/generated/gui_generated/src/containers/MenuItemBase.d \
./TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.d \
./TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.d \
./TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.d \
./TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.d \
./TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.d \
./TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/gui_generated/src/containers/%.o TouchGFX/generated/gui_generated/src/containers/%.su: ../TouchGFX/generated/gui_generated/src/containers/%.cpp TouchGFX/generated/gui_generated/src/containers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers

clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers:
	-$(RM) ./TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.d ./TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.o ./TouchGFX/generated/gui_generated/src/containers/CustomGaugeBase.su ./TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.d ./TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.o ./TouchGFX/generated/gui_generated/src/containers/EffectListItemBase.su ./TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.d ./TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.o ./TouchGFX/generated/gui_generated/src/containers/EffectListItemSelectedBase.su ./TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.d ./TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.o ./TouchGFX/generated/gui_generated/src/containers/EffectPictogramBase.su ./TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.d ./TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.o ./TouchGFX/generated/gui_generated/src/containers/EffectsListContainerBase.su ./TouchGFX/generated/gui_generated/src/containers/MenuItemBase.d ./TouchGFX/generated/gui_generated/src/containers/MenuItemBase.o ./TouchGFX/generated/gui_generated/src/containers/MenuItemBase.su ./TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.d ./TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.o ./TouchGFX/generated/gui_generated/src/containers/ModalWindowDeleteBase.su ./TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.d ./TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.o ./TouchGFX/generated/gui_generated/src/containers/SettingsItemBase.su ./TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.d ./TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.o ./TouchGFX/generated/gui_generated/src/containers/SubMenuContainerBase.su ./TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.d ./TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.o ./TouchGFX/generated/gui_generated/src/containers/SubMenuItemBase.su ./TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.d ./TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.o ./TouchGFX/generated/gui_generated/src/containers/SubSettingsContainerBase.su ./TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.d ./TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.o ./TouchGFX/generated/gui_generated/src/containers/WiringChangeBoxBase.su

.PHONY: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers

