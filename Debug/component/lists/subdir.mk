################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/lists/fsl_component_generic_list.c 

C_DEPS += \
./component/lists/fsl_component_generic_list.d 

OBJS += \
./component/lists/fsl_component_generic_list.o 


# Each subdirectory must supply rules for building sources it contributes
component/lists/%.o: ../component/lists/%.c component/lists/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK66FN2M0VMD18 -DCPU_MK66FN2M0VMD18_cm4 -DPRINTF_ADVANCED_ENABLE=1 -DFRDM_K66F -DFREEDOM -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\source" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\utilities" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\drivers" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\device" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\component\uart" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\component\lists" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\CMSIS" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\board" -I"C:\Users\bukre\Documents\MCUXpressoIDE_11.10.0_3148\workspace\FinalProject_SEH500\frdmk66f\driver_examples\i2c\read_accel_value_transfer" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections  -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-lists

clean-component-2f-lists:
	-$(RM) ./component/lists/fsl_component_generic_list.d ./component/lists/fsl_component_generic_list.o

.PHONY: clean-component-2f-lists

