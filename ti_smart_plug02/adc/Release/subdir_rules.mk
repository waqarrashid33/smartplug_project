################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
adc_userinput.obj: ../adc_userinput.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="adc_userinput.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

pinmux.obj: ../pinmux.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="pinmux.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

startup_ccs.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/startup_ccs.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="startup_ccs.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

uart_if.obj: /home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common/uart_if.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -Ooff --include_path="/home/waqar/ti/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/example/common" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/driverlib" --include_path="/home/waqar/ti/CC3200SDK_1.1.0/cc3200-sdk/inc" --define=ccs --define=cc3200 -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="uart_if.d" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


