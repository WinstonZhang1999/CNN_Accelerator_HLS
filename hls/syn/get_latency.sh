#!/bin/bash

ARCHS="SMALL MEDIUM FAST"
IMAGE=$1
TEAM_DEF=team.tcl
LAYER=$(cat $TEAM_DEF | grep "set" | cut -d '"' -f 2 | cut -d "_" -f 3)
for arc in $ARCHS; do

    # Get latency of each architecture
    LATENCY_TRACE="${PWD}/bdw_work/trace/sim.${arc}_ACCELERATED_TARGET_LAYER_${LAYER}_${IMAGE}_V.trace"
    LATENCY_LOG="${PWD}/latency_${arc}.log"

    if test ! -e $LATENCY_TRACE; then
        echo "--- $LATENCY_TRACE not found ---"
        continue
    fi

    if grep -q "Errors" $LATENCY_TRACE; then
        echo "--- Simulation for $arc failed ---"
        continue
    fi

    # NOTE: the numbers for time beg/end have to be modified if the format is changed
    # use //@ to remove the @ character at the front
    INFO_DRIVER_BEG=$(cat $LATENCY_TRACE | grep "Info: driver: " | grep "BEGIN - ACC")
    ARR_BEG=($INFO_DRIVER_BEG)
    TIME_BEG=${ARR_BEG[2]//@}

    if echo $INFO_DRIVER_BEG | grep -q "ps"; then
        TIME_BEG=$(echo "$TIME_BEG / 1000" | bc)
    fi

    if echo $INFO_DRIVER_BEG | grep -q "us"; then
        TIME_BEG=$(echo "$TIME_BEG * 1000" | bc)
    fi

    INFO_DRIVER_END=$(cat $LATENCY_TRACE | grep "Info: driver: " | grep "END - ACC")
    INFO_LOAD_END=$(cat $LATENCY_TRACE | grep "Info: driver: " | grep "LOAD - TIME")
    INFO_COMPUTE_END=$(cat $LATENCY_TRACE | grep "Info: driver: " | grep "COMPUTE - TIME")
    INFO_STORE_END=$(cat $LATENCY_TRACE | grep "Info: driver: " | grep "STORE - TIME")

    ARR_END=($INFO_DRIVER_END)
    LOAD_END=($INFO_LOAD_END)
    COMPUTE_END=($INFO_COMPUTE_END)
    STORE_END=($INFO_STORE_END)

    TIME_END=${ARR_END[2]//@}
    TIME_LOAD=${LOAD_END[2]//@}
    TIME_COMPUTE=${COMPUTE_END[2]//@}
    TIME_STORE=${STORE_END[2]//@}


    if echo $TIME_END | grep -q "e+"; then
        exp=$(echo $TIME_END | cut -d "+" -f 2)
        base=$(echo $TIME_END | cut -d "e" -f 1)
        TIME_END=$(echo "$base * (10 ^ $exp) / 1" | bc)
    fi

    if echo $INFO_DRIVER_END | grep -q "ps"; then
        TIME_END=$(echo "$TIME_END / 1000" | bc)
    fi

    if echo $INFO_DRIVER_END | grep -q "us"; then
        TIME_END=$(echo "$TIME_END * 1000" | bc)
    fi


    if echo $TIME_LOAD | grep -q "e+"; then
        exp=$(echo $TIME_LOAD | cut -d "+" -f 2)
        base=$(echo $TIME_LOAD | cut -d "e" -f 1)
        TIME_LOAD=$(echo "$base * (10 ^ $exp) / 1" | bc)
    fi

    if echo $INFO_LOAD_END | grep -q "ps"; then
        TIME_LOAD=$(echo "$TIME_LOAD / 1000" | bc)
    fi

    if echo $INFO_LOAD_END | grep -q "us"; then
        TIME_LOAD=$(echo "$TIME_LOAD * 1000" | bc)
    fi


    if echo $TIME_COMPUTE | grep -q "e+"; then
        exp=$(echo $TIME_COMPUTE | cut -d "+" -f 2)
        base=$(echo $TIME_COMPUTE | cut -d "e" -f 1)
        TIME_COMPUTE=$(echo "$base * (10 ^ $exp) / 1" | bc)
    fi

    if echo $INFO_COMPUTE_END | grep -q "ps"; then
        TIME_COMPUTE=$(echo "$TIME_COMPUTE / 1000" | bc)
    fi

    if echo $INFO_COMPUTE_END | grep -q "us"; then
        TIME_LOAD=$(echo "$TIME_COMPUTE * 1000" | bc)
    fi


    if echo $TIME_STORE | grep -q "e+"; then
        exp=$(echo $TIME_STORE | cut -d "+" -f 2)
        base=$(echo $TIME_STORE | cut -d "e" -f 1)
        TIME_STORE=$(echo "$base * (10 ^ $exp) / 1" | bc)
    fi

    if echo $INFO_STORE_END | grep -q "ps"; then
        TIME_STORE=$(echo "$TIME_STORE / 1000" | bc)
    fi

    if echo $INFO_STORE_END | grep -q "us"; then
        TIME_STORE=$(echo "$TIME_STORE * 1000" | bc)
    fi


    TOT_TIME=$(echo "$TIME_END - $TIME_BEG" | bc)
    echo "latency: $TOT_TIME ns"
    echo "load time: $TIME_LOAD ns"
    echo "compute time: $TIME_COMPUTE ns"
    echo "store time: $TIME_STORE ns"

    rm -f $LATENCY_LOG
    echo "$TOT_TIME" >> $LATENCY_LOG
    echo "$TIME_LOAD" >> $LATENCY_LOG
    echo "$TIME_COMPUTE" >> $LATENCY_LOG
    echo "$TIME_STORE" >> $LATENCY_LOG

done
