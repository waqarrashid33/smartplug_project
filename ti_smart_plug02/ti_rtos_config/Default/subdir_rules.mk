################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
build-11648687:
	@$(MAKE) -Onone -f subdir_rules.mk build-11648687-inproc

build-11648687-inproc: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/ti_rtos/ti_rtos_config/app.cfg
	@echo 'Building file: $<'
	@echo 'Invoking: XDCtools'
	"/home/waqar/ti/xdctools_3_32_01_22_core/xs" --xdcpath="/home/waqar/ti/ccsv7/ccs_base;/home/waqar/ti/tirtos_simplelink_2_01_00_03/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/bios_6_40_03_39/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/uia_2_00_01_34/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M4 -p ti.platforms.simplelink:CC3200 -r release -c "/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS" "$<"
	@echo 'Finished building: $<'
	@echo ' '

configPkg/linker.cmd: build-11648687 /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/ti_rtos/ti_rtos_config/app.cfg
configPkg/compiler.opt: build-11648687
configPkg/: build-11648687


