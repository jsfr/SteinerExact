#!/bin/bash

timeout="12h"

echo -e "{"
{
    /usr/bin/timeout $timeout /usr/bin/time -f 'time: %U' ${*:1};
    if [ "$?" = "124" ]; then
        echo -e "\ntrees: -1\niterations: -1\ntime: NaN\nsuccess: false"
    else
        echo "success: true"
    fi
} 2>&1 | \
    grep -E "(nodes|trees|iterations|time|success):|NUMSITES" | \
    sed "s/NUMSITES = /nodes: /g" | \
    tail -5 | \
    sed 's/^\(.*\): \(.*\)$/\t"\1": \2,/g;$s/,$//g'
echo -e "}"
