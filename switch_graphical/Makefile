# RIKITU devkitPro adaptation of switchbrew/switch-examples application template
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile (Optional)
#
# NO_ICON: if set to anything, do not use icon.
# NO_NACP: if set to anything, no .nacp file is generated.
# APP_TITLE is the name of the app stored in the .nacp file (Optional)
# APP_AUTHOR is the author of the app stored in the .nacp file (Optional)
# APP_VERSION is the version of the app stored in the .nacp file (Optional)
# APP_TITLEID is the titleID of the app stored in the .nacp file (Optional)
# ICON is the filename of the icon (.jpg), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.jpg
#     - icon.jpg
#     - <libnx folder>/default_icon.jpg
#
# CONFIG_JSON is the filename of the NPDM config file (.json), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.json
#     - config.json
#   If a JSON file is provided or autodetected, an ExeFS PFS0 (.nsp) is built instead
#   of a homebrew executable (.nro). This is intended to be used for sysmodules.
#   NACP building is skipped as well.
#---------------------------------------------------------------------------------
# QA_MODE=1 builds a separate, asset-free NeXium combat test binary.
ifeq ($(QA_MODE),6)
TARGET       := rikitu_staged_codecs_qa_probe
else ifeq ($(QA_MODE),5)
TARGET       := rikitu_staged_rename_qa_probe
else ifeq ($(QA_MODE),4)
TARGET       := rikitu_campaign_qa_probe
else ifeq ($(QA_MODE),2)
TARGET       := rikitu_save_qa_probe
else ifeq ($(QA_MODE),1)
TARGET       := rikitu_combat_qa_probe
else
TARGET       := rikitu_scene_probe
endif
APP_TITLE := RIKITU 2D Scene Probe
APP_AUTHOR := RIKITU LAB
APP_VERSION := 0.1.0
ifeq ($(QA_MODE),6)
BUILD        := build_staged_codecs_qa
else ifeq ($(QA_MODE),5)
BUILD        := build_staged_rename_qa
else ifeq ($(QA_MODE),4)
BUILD        := build_campaign_qa
else ifeq ($(QA_MODE),2)
BUILD        := build_save_qa
else ifeq ($(QA_MODE),1)
BUILD        := build_combat_qa
else
BUILD		:=	build
endif
SOURCES      := source ../native
DATA         :=
INCLUDES     := ../native
#ROMFS	:=	romfs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lnx -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

ifeq ($(QA_MODE),6)
CFILES := staged_codecs_qa_main.c rikitu_rules.c rikitu_loop.c rikitu_surfaces.c rikitu_stage.c rikitu_demo.c rikitu_flower.c rikitu_bridge.c rikitu_enemies.c rikitu_boss.c rikitu_campaign.c rikitu_save.c rikitu_campaign_save.c rikitu_level_save.c rikitu_staged_slot.c
else ifeq ($(QA_MODE),5)
CFILES := staged_rename_qa_main.c rikitu_staged_slot.c
else ifeq ($(QA_MODE),4)
CFILES := campaign_qa_main.c rikitu_rules.c rikitu_loop.c rikitu_surfaces.c rikitu_stage.c rikitu_demo.c rikitu_bridge.c rikitu_enemies.c rikitu_boss.c rikitu_campaign.c rikitu_campaign_save.c rikitu_campaign_save_io.c rikitu_level_save.c rikitu_level_save_io.c
else ifeq ($(QA_MODE),2)
CFILES := save_qa_main.c rikitu_rules.c rikitu_loop.c rikitu_surfaces.c rikitu_stage.c rikitu_demo.c rikitu_flower.c rikitu_enemies.c rikitu_boss.c rikitu_save.c rikitu_save_io.c
else ifeq ($(QA_MODE),1)
CFILES := combat_qa_main.c rikitu_rules.c rikitu_loop.c rikitu_surfaces.c rikitu_stage.c rikitu_draw.c rikitu_demo.c rikitu_flower.c rikitu_enemies.c rikitu_boss.c rikitu_combat_guest_qa.c rikitu_mill.c rikitu_mill_guest_qa.c rikitu_grove.c rikitu_grove_guest_qa.c rikitu_bridge.c rikitu_bridge_guest_qa.c rikitu_workshop.c rikitu_workshop_guest_qa.c rikitu_delivery.c rikitu_delivery_guest_qa.c
else
CFILES := main.c rikitu_rules.c rikitu_loop.c rikitu_surfaces.c rikitu_stage.c rikitu_draw.c rikitu_demo.c rikitu_flower.c rikitu_enemies.c rikitu_boss.c rikitu_mill.c rikitu_save.c rikitu_save_io.c rikitu_grove.c rikitu_bridge.c rikitu_workshop.c rikitu_delivery.c rikitu_clock.c rikitu_orders.c rikitu_mist.c rikitu_cloud.c rikitu_crystal.c rikitu_mine.c rikitu_gardens.c rikitu_foundry.c rikitu_gallery.c rikitu_heart.c rikitu_campaign.c rikitu_campaign_save.c rikitu_campaign_save_io.c rikitu_level_save.c rikitu_level_save_io.c
endif
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
ifeq ($(strip $(APP_JSON)),)
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
	@rm -fr $(BUILD) $(TARGET).nsp $(TARGET).nso $(TARGET).npdm $(TARGET).elf
endif


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(APP_JSON)),)

all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro	:	$(OUTPUT).elf
endif

else

all	:	$(OUTPUT).nsp

$(OUTPUT).nsp	:	$(OUTPUT).nso $(OUTPUT).npdm

$(OUTPUT).nso	:	$(OUTPUT).elf

endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
