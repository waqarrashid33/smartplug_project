#
#  Do not edit this file.  This file is generated from 
#  package.bld.  Any modifications to this file will be 
#  overwritten whenever makefiles are re-generated.
#
#  target compatibility key = ti.targets.arm.elf.M4{1,0,16.9,1
#
ifeq (,$(MK_NOGENDEPS))
-include package/cfg/app_pem4.oem4.dep
package/cfg/app_pem4.oem4.dep: ;
endif

package/cfg/app_pem4.oem4: | .interfaces
package/cfg/app_pem4.oem4: package/cfg/app_pem4.c package/cfg/app_pem4.mak
	@$(RM) $@.dep
	$(RM) $@
	@$(MSG) clem4 $< ...
	$(ti.targets.arm.elf.M4.rootDir)/bin/armcl -c  -qq -pdsw225 -ms --fp_mode=strict --endian=little -mv7M4 --float_support=vfplib --abi=eabi -eo.oem4 -ea.sem4   -Dxdc_cfg__xheader__='"configPkg/package/cfg/app_pem4.h"'  -Dxdc_target_name__=M4 -Dxdc_target_types__=ti/targets/arm/elf/std.h -Dxdc_bld__profile_release -Dxdc_bld__vers_1_0_16_9_1 -O2  $(XDCINCS) -I$(ti.targets.arm.elf.M4.rootDir)/include/rts -I$(ti.targets.arm.elf.M4.rootDir)/include  -fs=./package/cfg -fr=./package/cfg -fc $<
	$(MKDEP) -a $@.dep -p package/cfg -s oem4 $< -C   -qq -pdsw225 -ms --fp_mode=strict --endian=little -mv7M4 --float_support=vfplib --abi=eabi -eo.oem4 -ea.sem4   -Dxdc_cfg__xheader__='"configPkg/package/cfg/app_pem4.h"'  -Dxdc_target_name__=M4 -Dxdc_target_types__=ti/targets/arm/elf/std.h -Dxdc_bld__profile_release -Dxdc_bld__vers_1_0_16_9_1 -O2  $(XDCINCS) -I$(ti.targets.arm.elf.M4.rootDir)/include/rts -I$(ti.targets.arm.elf.M4.rootDir)/include  -fs=./package/cfg -fr=./package/cfg
	-@$(FIXDEP) $@.dep $@.dep
	
package/cfg/app_pem4.oem4: export C_DIR=
package/cfg/app_pem4.oem4: PATH:=$(ti.targets.arm.elf.M4.rootDir)/bin/:$(PATH)

package/cfg/app_pem4.sem4: | .interfaces
package/cfg/app_pem4.sem4: package/cfg/app_pem4.c package/cfg/app_pem4.mak
	@$(RM) $@.dep
	$(RM) $@
	@$(MSG) clem4 -n $< ...
	$(ti.targets.arm.elf.M4.rootDir)/bin/armcl -c -n -s --symdebug:none -qq -pdsw225 --endian=little -mv7M4 --float_support=vfplib --abi=eabi -eo.oem4 -ea.sem4   -Dxdc_cfg__xheader__='"configPkg/package/cfg/app_pem4.h"'  -Dxdc_target_name__=M4 -Dxdc_target_types__=ti/targets/arm/elf/std.h -Dxdc_bld__profile_release -Dxdc_bld__vers_1_0_16_9_1 -O2  $(XDCINCS) -I$(ti.targets.arm.elf.M4.rootDir)/include/rts -I$(ti.targets.arm.elf.M4.rootDir)/include  -fs=./package/cfg -fr=./package/cfg -fc $<
	$(MKDEP) -a $@.dep -p package/cfg -s oem4 $< -C  -n -s --symdebug:none -qq -pdsw225 --endian=little -mv7M4 --float_support=vfplib --abi=eabi -eo.oem4 -ea.sem4   -Dxdc_cfg__xheader__='"configPkg/package/cfg/app_pem4.h"'  -Dxdc_target_name__=M4 -Dxdc_target_types__=ti/targets/arm/elf/std.h -Dxdc_bld__profile_release -Dxdc_bld__vers_1_0_16_9_1 -O2  $(XDCINCS) -I$(ti.targets.arm.elf.M4.rootDir)/include/rts -I$(ti.targets.arm.elf.M4.rootDir)/include  -fs=./package/cfg -fr=./package/cfg
	-@$(FIXDEP) $@.dep $@.dep
	
package/cfg/app_pem4.sem4: export C_DIR=
package/cfg/app_pem4.sem4: PATH:=$(ti.targets.arm.elf.M4.rootDir)/bin/:$(PATH)

clean,em4 ::
	-$(RM) package/cfg/app_pem4.oem4
	-$(RM) package/cfg/app_pem4.sem4

app.pem4: package/cfg/app_pem4.oem4 package/cfg/app_pem4.mak

clean::
	-$(RM) package/cfg/app_pem4.mak
