################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./android_task.obj" \
"./cc3200.obj" \
"./clienthandler.obj" \
"./exosite.obj" \
"./exosite_hal.obj" \
"./exosite_meta.obj" \
"./exosite_task.obj" \
"./gpio_if.obj" \
"./main.obj" \
"./metrology.obj" \
"./nvmem_api.obj" \
"./pinmux.obj" \
"./smartconfig.obj" \
"./smartplug_task.obj" \
"./startup_ccs.obj" \
"./strlib.obj" \
"./uart_logger.obj" \
"./udmadrv.obj" \
"../cc3200R1.cmd" \
$(GEN_CMDS__FLAG) \
-lfree_rtos.a \
-lsimplelink.a \
-ldriverlib.a \
-llibc.a \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include exe/subdir_vars.mk
-include subdir_rules.mk
-include exe/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out \

EXE_OUTPUTS__QUOTED += \
"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out" \

BIN_OUTPUTS += \
/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.bin \

BIN_OUTPUTS__QUOTED += \
"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.bin" \


# All Target
all: /home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out

# Tool invocations
/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --float_support=vfplib -me -O2 --opt_for_speed=2 --fp_mode=strict --define=METROLOGY_DEBUG --define=ccs --define=USE_FREERTOS --define=SL_PLATFORM_MULTI_THREADED --define=cc3200 --printf_support=full --diag_suppress=1 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --ual -z -m"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.map" --heap_size=0x10000 --stack_size=0x2000 --cinit_hold_wdt=off -i"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/lib" -i"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" -i"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/simplelink/ccs/Exe" -i"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/driverlib/ccs/Exe" -i"/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/oslib/ccs/Exe" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="smartplug-gen1_linkInfo.xml" --rom_model --unused_section_elimination=on -o "/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.bin: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armhex"  -o "/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.bin" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "android_task.obj" "cc3200.obj" "clienthandler.obj" "exosite.obj" "exosite_hal.obj" "exosite_meta.obj" "exosite_task.obj" "gpio_if.obj" "main.obj" "metrology.obj" "nvmem_api.obj" "pinmux.obj" "smartconfig.obj" "smartplug_task.obj" "startup_ccs.obj" "strlib.obj" "uart_logger.obj" "udmadrv.obj" 
	-$(RM) "android_task.d" "cc3200.d" "clienthandler.d" "exosite.d" "exosite_hal.d" "exosite_meta.d" "exosite_task.d" "gpio_if.d" "main.d" "metrology.d" "nvmem_api.d" "pinmux.d" "smartconfig.d" "smartplug_task.d" "startup_ccs.d" "strlib.d" "uart_logger.d" "udmadrv.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-"/home/waqar/ti/ccsv7/utils/tiobj2bin/tiobj2bin" "/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.out" "/home/waqar/workspace/ti_smart_plug02/CC3200SmartPlug_1.0.0/src/ccs/exe/smartplug-gen1_freertos.bin" "/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armofd" "/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armhex" "/home/waqar/ti/ccsv7/utils/tiobj2bin/mkhex4bin"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

