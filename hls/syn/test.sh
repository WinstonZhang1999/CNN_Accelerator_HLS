#!/bin/bash

# Usage:
# ./test.sh <image> <arch> <layer>
#   e.g. ./test.sh cat SMALL TARGET_LAYER_1

if [[ $# < 3 ]] ; then
    echo 'usage: ./test.sh <image> <arch> <layer>'
    echo ' e.g.: ./test.sh cat SMALL TARGET_LAYER_1'
    exit 1
fi

# Animal (cat, etc.)
image=$1

# Arch (SMALL, MEDIUM, FAST)
arch=$2

# Layer (TARGET_LAYER_X)
layer=$3

mkdir -p test
mkdir -p native_results

LOGFILE=test/test_${arch}_${layer}_${image}.log

if test -e $LOGFILE; then
    mv $LOGFILE $LOGFILE.bak
fi

echo "======================" >> $LOGFILE
echo "=== Automated test ===" >> $LOGFILE
echo "======================" >> $LOGFILE
echo "" >> $LOGFILE

echo "=== BEHAV_NATIVE sim ===" >> $LOGFILE

# Create programmer view results file

echo -n "  - PV $image ... " >> $LOGFILE
src=../../pv
dst=${PWD}
num=$(echo "${layer: -1}")

cd $src
make IMAGE=$image TARGET_LAYER=$num dwarf-run
mv test.txt $dst/native_results/native_${layer}_expected.txt
cd $dst

echo "" >> $LOGFILE

# Native floating point
echo -n "  - native float behavioral $image ... " >> $LOGFILE
timeout --signal=KILL 30m make sim_BEHAV_NATIVE_${arch}_${layer}_${image} -j
sim_log=bdw_work/sims/BEHAV_NATIVE_${arch}_${layer}_${image}/bdw_sim.log
mv test.txt test/test_native_${arch}_${layer}_${image}.txt
/usr/bin/diff -q test/test_native_${arch}_${layer}_${image}.txt native_results/native_${layer}_expected.txt > /dev/null
r=$?
if [ $r -eq 0 ]; then
    echo "Native simulation PASSED" >> $LOGFILE
else
    echo "Native simulation FAILED (Aborting)" >> $LOGFILE
    exit
fi

echo "" >> $LOGFILE

# Component accuracy in fixed-point software
echo "Test component fixed-point accuracy in software" >> $LOGFILE

rm -f ../../accuracy/accuracy_comp_res.log
timeout --signal=KILL 60m make accuracy_comp_${arch}
if [ -e "../../accuracy/accuracy_comp_res.log" ]; then
    echo "" >> $LOGFILE
    cat ../../accuracy/accuracy_comp_res.log >> $LOGFILE
else
    echo -n "Layer accuracy test ERROR" >> $LOGFILE
fi


# Fixed-point accelerated
timeout --signal=KILL 30m make sim_BEHAV_ACCELERATED_${arch}_${layer}_${image} -j
mv accelerated_test.txt test/accelerated_test_beh_${arch}_${layer}_${image}.txt

echo "" >> $LOGFILE

# HLS
echo "=== ${arch} ===" >> $LOGFILE
echo -n "  - HLS ${arch} ... " >> $LOGFILE
timeout --signal=KILL 2h make hls_${arch}
hls_log=bdw_work/modules/conv_layer/${arch}/stratus_hls.log
grep -i "stratus_hls succeeded with 0 errors" $hls_log
r=$?
if [ $r -eq 0 ]; then
    echo "PASSED" >> $LOGFILE
else
    echo "FAILED (skip simulation)" >> $LOGFILE
    echo "" >> $LOGFILE
    exit
fi

echo "    == RTL sim ==" >> $LOGFILE

# RTL accelerted
echo -n "      - RTL ${arch} ... " >> $LOGFILE
timeout --signal=KILL 60m make sim_${arch}\_ACCELERATED_${layer}_${image}_V -j
mv accelerated_test.txt test/accelerated_test_rtl_${arch}_${layer}_${image}.txt
/usr/bin/diff -q test/accelerated_test_rtl_${arch}_${layer}_${image}.txt test/accelerated_test_beh_${arch}_${layer}_${image}.txt > /dev/null
r=$?
if [ $r -eq 0 ]; then
    echo "PASSED" >> $LOGFILE
else
    echo "FAILED (Aborting)" >> $LOGFILE
    exit
fi

echo "" >> $LOGFILE
