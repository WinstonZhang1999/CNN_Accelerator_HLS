# Copyright 2021 Columbia University, SLD Group

#
# Technology Libraries
#

set MEMTECH_PATH ../../scripts/memgen/virtex7
set VIVADO $::env(XILINX_VIVADO)

set_attr fpga_use_dsp off
set_attr fpga_tool "vivado"
set_attr fpga_part "xczu9eg-ffvb1156-2-e"

set_attr verilog_files "$MEMTECH_PATH/*.v"
set_attr verilog_files "$VIVADO/data/verilog/src/glbl.v"
set_attr verilog_files "$VIVADO/data/verilog/src/retarget/RAMB*.v"
set_attr verilog_files "$VIVADO/data/verilog/src/unisims/RAMB*.v"

#
# Set the library for the memories
#

use_hls_lib "./memlib"

#
# Global project includes
#

set INCLUDES "-I../src -I./memlib"

#
# High-Level Synthesis Options
#

set_attr hls_cc_options "$INCLUDES"

#
# Compiler and Simulator Options
#

use_systemc_simulator incisive
set_attr cc_options "$INCLUDES -O3"
set_attr end_of_sim_command "make saySimPassed"

#
# Fixed global syntheis attributes
#

set_attr message_detail           2
set_attr default_input_delay      0.1
set_attr default_protocol         false
set_attr inline_partial_constants true
set_attr output_style_reset_all   true
set_attr lsb_trimming             true

#
# Target layers based on assigned layer (DO NOT EDIT)
#

source ./team.tcl

# AXI4 - Bit width of identifiers

set ID_WIDTH "6"

# AXI4 - Bit width for the addresses

set ADDR_WIDTH "32"

#
# Testbench or system level modules
#

define_system_module tb ../tb/driver.cpp \
                        ../tb/memory.cpp \
                        ../tb/system.cpp \
                        ../tb/sc_main.cpp


################################################################################
# DO NOT EDIT ABOVE THIS LINE
################################################################################

#
# DSE configurations
#

# AXI4 - Bit width for the data values (assuming no less than 32)

set DATA_WIDTH_SMALL  "32"
set DATA_WIDTH_MEDIUM "32"
set DATA_WIDTH_FAST   "32"

# Size of the private local memory (fully-connected)

set PLM_SIZE_SMALL  "2048"
set PLM_SIZE_MEDIUM "2048"
set PLM_SIZE_FAST   "2048"

# Size of the private local memory (convolutional)

set PLM_INPUT_SIZE_SMALL 51200
set PLM_INPUT_WEIGHT_SIZE_SMALL 12
set PLM_OUTPUT_BIAS_SIZE_SMALL 512
set PLM_OUTPUT_SIZE_SMALL $PLM_INPUT_SIZE_SMALL

set PLM_INPUT_SIZE_MEDIUM 51200
set PLM_INPUT_WEIGHT_SIZE_MEDIUM 12
set PLM_OUTPUT_BIAS_SIZE_MEDIUM 512
set PLM_OUTPUT_SIZE_MEDIUM $PLM_INPUT_SIZE_MEDIUM

set PLM_INPUT_SIZE_FAST 51200
set PLM_INPUT_WEIGHT_SIZE_FAST 12
set PLM_OUTPUT_BIAS_SIZE_FAST 512
set PLM_OUTPUT_SIZE_FAST $PLM_INPUT_SIZE_FAST

# Set clock/reset period (ns)

set CLOCK_PERIOD_SMALL  10.0
set CLOCK_PERIOD_MEDIUM 10.0
set CLOCK_PERIOD_FAST   5

set RESET_PERIOD_SMALL  [expr $CLOCK_PERIOD_SMALL * 30]
set RESET_PERIOD_MEDIUM [expr $CLOCK_PERIOD_MEDIUM * 30]
set RESET_PERIOD_FAST   [expr $CLOCK_PERIOD_FAST * 30]

# Global synthesis attributes
set_attr sharing_effort_regs      low
set_attr sharing_effort_parts     low

# Synthesis flags
set HLS_FLAGS_SMALL  "--prints=off --clock_period=$CLOCK_PERIOD_SMALL  --sched_effort=low --sched_asap=on"
set HLS_FLAGS_MEDIUM "--prints=off --clock_period=$CLOCK_PERIOD_MEDIUM --sched_effort=low --sched_asap=on"
set HLS_FLAGS_FAST   "--prints=off --clock_period=$CLOCK_PERIOD_FAST   --sched_effort=low --sched_asap=on --timing_aggression=10 --unroll_loops=on --dpopt_auto=all --flatten_arrays=all" 
# pg681, dpopt_auto
################################################################################
# DO NOT EDIT BELOW THIS LINE
################################################################################

set fully_connected "false"
set LAYER_TYPE_TEST TEST_CONVOLUTION
foreach l $TARGET_LAYERS {
    if {$l eq "TARGET_LAYER_5" || $l eq "TARGET_LAYER_6"} {
	set fully_connected "true"
	set LAYER_TYPE_TEST TEST_FULLY_CONNECTED
    }
}

#
# System level modules to be synthesized
#

if {$fully_connected ne "false"} {
    set target fc_layer
    define_hls_module $target ../src/fc_layer.cpp
} else {
    set target conv_layer
    define_hls_module $target ../src/conv_layer.cpp
}

# ioconfig flags
set CFG_FLAGS_SMALL  "-DCLOCK_PERIOD=$CLOCK_PERIOD_SMALL  -DRESET_PERIOD=$RESET_PERIOD_SMALL  -D$LAYER_TYPE_TEST"
set CFG_FLAGS_MEDIUM "-DCLOCK_PERIOD=$CLOCK_PERIOD_MEDIUM -DRESET_PERIOD=$RESET_PERIOD_MEDIUM -D$LAYER_TYPE_TEST"
set CFG_FLAGS_FAST   "-DCLOCK_PERIOD=$CLOCK_PERIOD_FAST   -DRESET_PERIOD=$RESET_PERIOD_FAST   -D$LAYER_TYPE_TEST"

#
# Input images
#

set IMAGES ""
append IMAGES "airplane "
append IMAGES "automobile "
append IMAGES "bird "
append IMAGES "cat "
append IMAGES "deer "
append IMAGES "dog "
append IMAGES "frog "
append IMAGES "horse "
append IMAGES "ship "
append IMAGES "truck "

#
# Generating sim/system configs
#
set TYPES "NATIVE FIXED"

foreach t $TYPES {
    define_io_config * IOCFG_$t\_SMALL \
	$CFG_FLAGS_SMALL \
	-DDATA_WIDTH=$DATA_WIDTH_SMALL \
	-DID_WIDTH=$ID_WIDTH \
	-DADDR_WIDTH=$ADDR_WIDTH \
	-DPLM_SIZE=$PLM_SIZE_SMALL \
	-DPLM_INPUT_SIZE=$PLM_INPUT_SIZE_SMALL \
	-DPLM_INPUT_WEIGHT_SIZE=$PLM_INPUT_WEIGHT_SIZE_SMALL \
	-DPLM_OUTPUT_BIAS_SIZE=$PLM_OUTPUT_BIAS_SIZE_SMALL \
	-DPLM_OUTPUT_SIZE=$PLM_OUTPUT_SIZE_SMALL \
	-D$t \
	-DDEBUG \
	-DSMALL

    define_io_config * IOCFG_$t\_MEDIUM \
	$CFG_FLAGS_MEDIUM \
	-DDATA_WIDTH=$DATA_WIDTH_MEDIUM \
	-DID_WIDTH=$ID_WIDTH \
	-DADDR_WIDTH=$ADDR_WIDTH \
	-DPLM_SIZE=$PLM_SIZE_MEDIUM \
	-DPLM_INPUT_SIZE=$PLM_INPUT_SIZE_MEDIUM \
	-DPLM_INPUT_WEIGHT_SIZE=$PLM_INPUT_WEIGHT_SIZE_MEDIUM \
	-DPLM_OUTPUT_BIAS_SIZE=$PLM_OUTPUT_BIAS_SIZE_MEDIUM \
	-DPLM_OUTPUT_SIZE=$PLM_OUTPUT_SIZE_MEDIUM \
	-D$t \
	-DDEBUG \
	-DMEDIUM

    define_io_config * IOCFG_$t\_FAST \
	$CFG_FLAGS_FAST \
	-DDATA_WIDTH=$DATA_WIDTH_FAST \
	-DID_WIDTH=$ID_WIDTH \
	-DADDR_WIDTH=$ADDR_WIDTH \
	-DPLM_SIZE=$PLM_SIZE_FAST \
	-DPLM_INPUT_SIZE=$PLM_INPUT_SIZE_FAST \
	-DPLM_INPUT_WEIGHT_SIZE=$PLM_INPUT_WEIGHT_SIZE_FAST \
	-DPLM_OUTPUT_BIAS_SIZE=$PLM_OUTPUT_BIAS_SIZE_FAST \
	-DPLM_OUTPUT_SIZE=$PLM_OUTPUT_SIZE_FAST \
	-D$t \
	-DDEBUG \
	-DFAST
}

#HLS configurations
define_hls_config $target SMALL  $HLS_FLAGS_SMALL  -io_config IOCFG_FIXED_SMALL
define_hls_config $target MEDIUM $HLS_FLAGS_MEDIUM -io_config IOCFG_FIXED_MEDIUM
define_hls_config $target FAST   $HLS_FLAGS_FAST   -io_config IOCFG_FIXED_FAST

set CFGS "SMALL MEDIUM FAST"

foreach cfg $CFGS {

    foreach l $TARGET_LAYERS {

	# Define the testbench
	define_system_config tb TESTBENCH_NATIVE_$cfg\_$l -DRUN_SIM -io_config IOCFG_NATIVE_$cfg -D$l

	define_system_config tb TESTBENCH_$cfg\_$l -DRUN_SIM -io_config IOCFG_FIXED_$cfg -D$l

	define_system_config tb TESTBENCH_ACCELERATED_$cfg\_$l -DRUN_ACCELERATED_SIM -io_config IOCFG_FIXED_$cfg -D$l

        define_system_config tb TESTBENCH_ACCELERATED_NATIVE_$cfg\_$l -DRUN_ACCELERATED_SIM -io_config IOCFG_NATIVE_$cfg -D$l


	foreach img $IMAGES {

	    # Behavioral simulation
	    define_sim_config "BEHAV_NATIVE_$cfg\_$l\_$img" "$target BEH" \
		"tb TESTBENCH_NATIVE_$cfg\_$l" \
		-argv $img

	    define_sim_config "BEHAV_$cfg\_$l\_$img" "$target BEH" \
		"tb TESTBENCH_$cfg\_$l" \
		-argv $img

	    define_sim_config "BEHAV_ACCELERATED_NATIVE_$cfg\_$l\_$img" "$target BEH" \
		"tb TESTBENCH_ACCELERATED_NATIVE_$cfg\_$l" \
		-argv $img

	    define_sim_config "BEHAV_ACCELERATED_$cfg\_$l\_$img" "$target BEH" \
		"tb TESTBENCH_ACCELERATED_$cfg\_$l" \
		-argv $img

	    # RTL simulation
	    define_sim_config "$cfg\_$l\_$img\_V" "$target RTL_V $cfg" \
		"tb TESTBENCH_$cfg\_$l" \
		-argv $img -verilog_top_modules glbl

	    define_sim_config "$cfg\_ACCELERATED_$l\_$img\_V"  "$target RTL_V $cfg"  \
		"tb TESTBENCH_ACCELERATED_$cfg\_$l" \
		-argv $img -verilog_top_modules glbl

	}; # foreach $IMAGES

    }; # foreach $TARGET_LAYERS

}; # foreach $CFGS
