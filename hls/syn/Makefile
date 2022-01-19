# Copyright 2021 Columbia University, SLD Group

#*******************************************************************************
#
# Makefile - top-level project makefile
#
#*******************************************************************************

# Define V=1 for a more verbose compilation
ifndef V
        QUIET_CXX           = @echo '   ' CXX $@;
        QUIET_LINK          = @echo '   ' LINK $@;
        QUIET_MKDIR         = @echo '   ' MKDIR $@;
        QUIET_RUN           = @echo '   ' RUN $@;
        QUIET_UNZIP         = @echo '   ' UNZIP $@;
        QUIET_WGET          = @echo '   ' WGET $@;
        QUIET_CLEAN         = @echo '   ' CLEAN $@;
        QUIET_MAKE         = @echo '   ' MAKE $@;
endif

# Default settings for automated test
TEST_IMAGE ?= cat
TEST_LAYER ?= TARGET_LAYER_4

HW_ROOT ?= $(shell realpath ../../)

INPUT_PATH ?= $(HW_ROOT)/scripts/preprocess

IMGS = $(wildcard $(INPUT_PATH)/*.bmp)
BMPS = $(foreach j, $(IMGS), $(shell basename $(j)))
BINS = $(BMPS:.bmp=.bin)

MODELS ?= $(HW_ROOT)/models
MOJO_PRETRAINED ?= $(MODELS)/dwarf7.mojo

OPENCV ?= /opt/opencv
HW_ROOT ?= ../..
PV ?= $(HW_ROOT)/pv
MODELS ?= $(HW_ROOT)/models

DMA_PRINT_OFF ?=

# CXX Flags
VERBOSE_DBG ?= 0
INCDIR ?=
INCDIR += -I$(PV)/mojo
INCDIR += -I../src -I../tb
CXXFLAGS += $(INCDIR)

BDW_EXTRA_CCFLAGS = $(CXXFLAGS)
BDW_EXTRA_HLFLAGS = $(CXXFLAGS)

# LD Flags
LDLIBS ?=
LDFLAGS ?=
BDW_EXTRA_LDFLAGS = $(LDLIBS)
BDW_EXTRA_LIB_FLAGS = $(LDFLAGS)

# list subdirectories that contain memory libraries
# (must be in subdirectories of the current directory)
MEMLIBS = ./memlib

MEMTECH_PATH = $(HW_ROOT)/scripts/memgen/virtex7

# list subdirectories that contain interface libraries
# (must be in subdirectories of the current directory)
IFLIBS =

# Uncomment this line to use the Insight debugger
#BDW_DEBUG = 1

saySimPassed:
	@bdw_sim_pass

# Disable timing checks and flush output to screen immediately
BDW_VLOGSIM_ARGS = -NOTIMINGCHECKS -UNBUFFERED

-include Makefile.prj

$(MOJO_PRETRAINED):
	@mkdir -p $(MODELS)
	@cp -f /opt/dwarf_model/dwarf7.mojo $@

$(INPUT_PATH)/preprocess:
	$(QUIET_MAKE) $(MAKE) -C $(INPUT_PATH)

$(BINS): $(INPUT_PATH)/preprocess
	@if ! test -e $@; then \
		ln -s $(INPUT_PATH)/$@; \
	fi;

Makefile.prj : $(BINS) $(MOJO_PRETRAINED) project.tcl  memlib team.tcl
	$(QUIET_MAKE) bdw_makegen

memlib: memlist.txt
	@rm -rf memlib/*
	@$(HW_ROOT)/scripts/memgen/memgen.py $(MEMTECH_PATH) memlist.txt ./memlib | tee memgen.log
	@bdw_memgen memlib/*.bdm &> /dev/null
	@echo "Memories generated successfully"


distclean: clean clean_all clean_libs
	@make -C $(INPUT_PATH) clean
	$(QUIET_CLEAN) rm -rf cachelib/*
	@rm -rf *.log

clean: clean_all clean_libs
	@rm -f *.bin
	@rm -rf memlib
	@rm -f transcript vsim* *.wlf data.out
	@rm -f *~ *.bak *.BAK
	@rm -rf work debussy* vfast*
	@rm -f Makefile.prj
	@rm -f msg_help.html
	@rm -rf core
	@rm -rf core.*
	@rm -rf .stack.*
	@rm -rf *irun.history*
	@rm -rf *.pro *.pro.user
	@rm -rf INCA_libs
	@rm -rf *.shm
	@rm -rf *.diag
	@rm -rf *.key
	@rm -rf *.csv
	@rm -rf .simvision
	@rm -rf .Xil
	@rm -rf .fail*
	@rm -rf *.bin
	@rm -rf *.err
	@rm -rf *.svcf
	@rm -rf *.ccpp
	@rm -rf .project.tcl.check
	@rm -rf .stratusIDEProjectSettings
	@rm -rf .syn.qws
	@rm -rf .stack.*
	@rm -rf accelerated_test*.txt
	@rm -rf memgen.log
	@rm -rf test/*
	@rm -rf native_results/*

.PHONY: clean distclean

test_SMALL:
	./test.sh $(TEST_IMAGE) SMALL $(TEST_LAYER)

test_MEDIUM:
	./test.sh $(TEST_IMAGE) MEDIUM $(TEST_LAYER)

test_FAST:
	./test.sh $(TEST_IMAGE) FAST $(TEST_LAYER)

accuracy_comp_SMALL:
	make accuracy_comp_SMALL -C ../../accuracy

accuracy_comp_MEDIUM:
	make accuracy_comp_MEDIUM -C ../../accuracy

accuracy_comp_FAST:
	make accuracy_comp_FAST -C ../../accuracy

.PHONY: test_SMALL test_MEDIUM test_FAST accuracy_comp_SMALL accuracy_comp_MEDIUM accuracy_comp_FAST
