################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-803998079: ../c2000.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs1281/ccs/utils/sysconfig_1.21.0/sysconfig_cli.bat" --script "E:/Coding/CCS/DAB_driver_tesing_V5_RS/c2000.syscfg" -o "syscfg" -s "C:/ti/c2000/C2000Ware_6_00_01_00/.metadata/sdk.json" -d "F280013x" --package 64PM --part F280013x_64PM --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/error.h: build-803998079 ../c2000.syscfg
syscfg: build-803998079

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 -Ooff --include_path="E:/Coding/CCS/DAB_driver_tesing_V5_RS" --include_path="C:/ti/c2000/C2000Ware_6_00_01_00" --include_path="E:/Coding/CCS/DAB_driver_tesing_V5_RS/device" --include_path="C:/ti/c2000/C2000Ware_6_00_01_00/driverlib/f280013x/driverlib" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --define=DEBUG --define=_FLASH --define=_LAUNCHXL_F2800137 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="E:/Coding/CCS/DAB_driver_tesing_V5_RS/CPU1_LAUNCHXL_FLASH/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


