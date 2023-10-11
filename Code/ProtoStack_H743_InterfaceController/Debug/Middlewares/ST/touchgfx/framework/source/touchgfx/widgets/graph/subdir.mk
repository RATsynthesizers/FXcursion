################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.cpp \
../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.cpp 

OBJS += \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.o \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.o 

CPP_DEPS += \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.d \
./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/%.o Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/%.su Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/%.cyclo: ../Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/%.cpp Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph

clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph:
	-$(RM) ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/Graph.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.su ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.cyclo ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.d ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.o ./Middlewares/ST/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.su

.PHONY: clean-Middlewares-2f-ST-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph

