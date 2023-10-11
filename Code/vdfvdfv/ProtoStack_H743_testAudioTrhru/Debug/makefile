################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Modules/libModules/subdir.mk
-include Modules/Effects/vibrato/subdir.mk
-include Modules/Effects/tremolo/subdir.mk
-include Modules/Effects/reverb/subdir.mk
-include Modules/Effects/phaser/subdir.mk
-include Modules/Effects/overdrive/subdir.mk
-include Modules/Effects/flanger/subdir.mk
-include Modules/Effects/distortion/subdir.mk
-include Modules/Effects/delay/subdir.mk
-include Modules/Effects/compressor/subdir.mk
-include Modules/Effects/chorus/subdir.mk
-include Modules/Effects/amp/subdir.mk
-include Modules/subdir.mk
-include Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk
-include Drivers/HW/subdir.mk
-include DSP_MY/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := ProtoStack_H743_testAudioTrhru_Dima
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
ProtoStack_H743_testAudioTrhru_Dima.elf \

MAP_FILES += \
ProtoStack_H743_testAudioTrhru_Dima.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
ProtoStack_H743_testAudioTrhru_Dima.list \

OBJCOPY_HEX += \
ProtoStack_H743_testAudioTrhru_Dima.hex \

OBJCOPY_BIN += \
ProtoStack_H743_testAudioTrhru_Dima.bin \


# All Target
all: main-build

# Main-build Target
main-build: ProtoStack_H743_testAudioTrhru_Dima.elf secondary-outputs

# Tool invocations
ProtoStack_H743_testAudioTrhru_Dima.elf ProtoStack_H743_testAudioTrhru_Dima.map: $(OBJS) $(USER_OBJS) D:\YandexDisk\_PEDAL_BreadBoard\Code\vdfvdfv\ProtoStack_H743_testAudioTrhru\STM32H743VITX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-g++ -o "ProtoStack_H743_testAudioTrhru_Dima.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"D:\YandexDisk\_PEDAL_BreadBoard\Code\vdfvdfv\ProtoStack_H743_testAudioTrhru\STM32H743VITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="ProtoStack_H743_testAudioTrhru_Dima.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

ProtoStack_H743_testAudioTrhru_Dima.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "ProtoStack_H743_testAudioTrhru_Dima.list"
	@echo 'Finished building: $@'
	@echo ' '

ProtoStack_H743_testAudioTrhru_Dima.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "ProtoStack_H743_testAudioTrhru_Dima.hex"
	@echo 'Finished building: $@'
	@echo ' '

ProtoStack_H743_testAudioTrhru_Dima.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "ProtoStack_H743_testAudioTrhru_Dima.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ProtoStack_H743_testAudioTrhru_Dima.bin ProtoStack_H743_testAudioTrhru_Dima.elf ProtoStack_H743_testAudioTrhru_Dima.hex ProtoStack_H743_testAudioTrhru_Dima.list ProtoStack_H743_testAudioTrhru_Dima.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
