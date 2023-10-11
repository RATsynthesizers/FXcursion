################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/containers/CustomGauge.cpp \
../TouchGFX/gui/src/containers/EffectListItem.cpp \
../TouchGFX/gui/src/containers/EffectListItemSelected.cpp \
../TouchGFX/gui/src/containers/EffectPictogram.cpp \
../TouchGFX/gui/src/containers/EffectsListContainer.cpp \
../TouchGFX/gui/src/containers/MenuItem.cpp \
../TouchGFX/gui/src/containers/ModalWindowDelete.cpp \
../TouchGFX/gui/src/containers/SettingsItem.cpp \
../TouchGFX/gui/src/containers/SubMenuContainer.cpp \
../TouchGFX/gui/src/containers/SubMenuItem.cpp \
../TouchGFX/gui/src/containers/SubSettingsContainer.cpp \
../TouchGFX/gui/src/containers/WiringChangeBox.cpp 

OBJS += \
./TouchGFX/gui/src/containers/CustomGauge.o \
./TouchGFX/gui/src/containers/EffectListItem.o \
./TouchGFX/gui/src/containers/EffectListItemSelected.o \
./TouchGFX/gui/src/containers/EffectPictogram.o \
./TouchGFX/gui/src/containers/EffectsListContainer.o \
./TouchGFX/gui/src/containers/MenuItem.o \
./TouchGFX/gui/src/containers/ModalWindowDelete.o \
./TouchGFX/gui/src/containers/SettingsItem.o \
./TouchGFX/gui/src/containers/SubMenuContainer.o \
./TouchGFX/gui/src/containers/SubMenuItem.o \
./TouchGFX/gui/src/containers/SubSettingsContainer.o \
./TouchGFX/gui/src/containers/WiringChangeBox.o 

CPP_DEPS += \
./TouchGFX/gui/src/containers/CustomGauge.d \
./TouchGFX/gui/src/containers/EffectListItem.d \
./TouchGFX/gui/src/containers/EffectListItemSelected.d \
./TouchGFX/gui/src/containers/EffectPictogram.d \
./TouchGFX/gui/src/containers/EffectsListContainer.d \
./TouchGFX/gui/src/containers/MenuItem.d \
./TouchGFX/gui/src/containers/ModalWindowDelete.d \
./TouchGFX/gui/src/containers/SettingsItem.d \
./TouchGFX/gui/src/containers/SubMenuContainer.d \
./TouchGFX/gui/src/containers/SubMenuItem.d \
./TouchGFX/gui/src/containers/SubSettingsContainer.d \
./TouchGFX/gui/src/containers/WiringChangeBox.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/containers/%.o TouchGFX/gui/src/containers/%.su: ../TouchGFX/gui/src/containers/%.cpp TouchGFX/gui/src/containers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-containers

clean-TouchGFX-2f-gui-2f-src-2f-containers:
	-$(RM) ./TouchGFX/gui/src/containers/CustomGauge.d ./TouchGFX/gui/src/containers/CustomGauge.o ./TouchGFX/gui/src/containers/CustomGauge.su ./TouchGFX/gui/src/containers/EffectListItem.d ./TouchGFX/gui/src/containers/EffectListItem.o ./TouchGFX/gui/src/containers/EffectListItem.su ./TouchGFX/gui/src/containers/EffectListItemSelected.d ./TouchGFX/gui/src/containers/EffectListItemSelected.o ./TouchGFX/gui/src/containers/EffectListItemSelected.su ./TouchGFX/gui/src/containers/EffectPictogram.d ./TouchGFX/gui/src/containers/EffectPictogram.o ./TouchGFX/gui/src/containers/EffectPictogram.su ./TouchGFX/gui/src/containers/EffectsListContainer.d ./TouchGFX/gui/src/containers/EffectsListContainer.o ./TouchGFX/gui/src/containers/EffectsListContainer.su ./TouchGFX/gui/src/containers/MenuItem.d ./TouchGFX/gui/src/containers/MenuItem.o ./TouchGFX/gui/src/containers/MenuItem.su ./TouchGFX/gui/src/containers/ModalWindowDelete.d ./TouchGFX/gui/src/containers/ModalWindowDelete.o ./TouchGFX/gui/src/containers/ModalWindowDelete.su ./TouchGFX/gui/src/containers/SettingsItem.d ./TouchGFX/gui/src/containers/SettingsItem.o ./TouchGFX/gui/src/containers/SettingsItem.su ./TouchGFX/gui/src/containers/SubMenuContainer.d ./TouchGFX/gui/src/containers/SubMenuContainer.o ./TouchGFX/gui/src/containers/SubMenuContainer.su ./TouchGFX/gui/src/containers/SubMenuItem.d ./TouchGFX/gui/src/containers/SubMenuItem.o ./TouchGFX/gui/src/containers/SubMenuItem.su ./TouchGFX/gui/src/containers/SubSettingsContainer.d ./TouchGFX/gui/src/containers/SubSettingsContainer.o ./TouchGFX/gui/src/containers/SubSettingsContainer.su ./TouchGFX/gui/src/containers/WiringChangeBox.d ./TouchGFX/gui/src/containers/WiringChangeBox.o ./TouchGFX/gui/src/containers/WiringChangeBox.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-containers

