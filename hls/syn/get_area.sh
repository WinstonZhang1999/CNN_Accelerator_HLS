#!/bin/bash

ARCHS="SMALL MEDIUM FAST"

MEMLIST_FILE=./memlist.txt
PLMS=$(cat $MEMLIST_FILE | cut -d " " -f 1)

for arc in $ARCHS; do

    # Get area of each architecture
    AREA_TRACE="${PWD}/bdw_work/trace/hls.conv_layer.${arc}.trace"
    AREA_LOG="${PWD}/area_${arc}.log"

    if test ! -e $AREA_TRACE; then
        echo "--- trace log of $arc not found ---"
        continue
    fi

    if grep -q "failed" $AREA_TRACE; then
        echo "---HLS for $arc failed---"
        continue
    fi

    LIST_LUTS=$(cat $AREA_TRACE | grep "Total LUTs/Mults" |  sed 's/\s\+/ /g'  | cut -d " " -f 6)
    LIST_MULTS=$(cat $AREA_TRACE | grep "Total LUTs/Mults" |  sed 's/\s\+/ /g'  | cut -d " " -f 7)

    ARR_LUTS=($LIST_LUTS)
    ARR_MULTS=($LIST_MULTS)

    AREA_BRAMS=0
    A_BRAMS_PLM=0

    # Grep all the PLMs one by one
    for plm in $PLMS; do
        LIST_BRAM=$(cat $AREA_TRACE | grep "00402" | grep $plm | sed 's/\s\+/ /g' | cut -d " " -f 8)
        # use the last item of the arrays LUTS/Mults
        A_BRAMS=$(( ${LIST_BRAM//$'\n'/+} ))
        A_BRAMS_PLM=$((A_BRAMS_PLM + A_BRAMS))
        # area formula: BRAM*0.00142 + LUTs*0.00000434 + Mults*0.000578704
        AREA_BRAMS=$(echo "$A_BRAMS_PLM*0.00142" | bc -l)

    done
    # use the last item of the arrays LUTS/Mults
    TEMP1=${ARR_LUTS[-1]}
    A_LUTS=$(echo "$TEMP1 - $A_BRAMS_PLM" | bc)
    A_MULTS=${ARR_MULTS[-1]}

    # area formula: BRAM*0.00142 + LUTs*0.00000434 + Mults*0.000578704
    AREA_LUTS=$(echo "$A_LUTS*0.00000434" | bc -l)
    AREA_MULTS=$(echo "$A_MULTS*0.000578704" | bc -l)

    TOT_AREA=$(echo "$AREA_BRAMS + $AREA_LUTS + $AREA_MULTS" | bc -l)
    AVG_AREA=$(echo "$TOT_AREA / 3" | bc -l)

    rm -f $AREA_LOG
    echo "0$AVG_AREA" >> $AREA_LOG
    echo "$A_BRAMS_PLM" >> $AREA_LOG
    echo "$A_LUTS" >> $AREA_LOG
    echo "$A_MULTS" >> $AREA_LOG

done
