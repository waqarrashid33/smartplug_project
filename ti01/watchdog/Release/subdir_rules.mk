################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
gpio_if.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/gpio_if.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="gpio_if.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

pinmux.obj: ../pinmux.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="pinmux.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

startup_ccs.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/startup_ccs.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="startup_ccs.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

uart_if.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/uart_if.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="uart_if.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

wdt_if.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/wdt_if.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --define=ccs --define=NON_NETWORK --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --display_error_number --abi=eabi --ual --preproc_with_compile --preproc_dependency="wdt_if.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


