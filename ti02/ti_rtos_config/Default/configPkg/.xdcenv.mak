#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = /home/waqar/ti/ccsv7/ccs_base;/home/waqar/ti/tirtos_simplelink_2_01_00_03/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/bios_6_40_03_39/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/uia_2_00_01_34/packages
override XDCROOT = /home/waqar/ti/xdctools_3_30_04_52_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = /home/waqar/ti/ccsv7/ccs_base;/home/waqar/ti/tirtos_simplelink_2_01_00_03/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/bios_6_40_03_39/packages;/home/waqar/ti/tirtos_simplelink_2_01_00_03/products/uia_2_00_01_34/packages;/home/waqar/ti/xdctools_3_30_04_52_core/packages;..
HOSTOS = Linux
endif
