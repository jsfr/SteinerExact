#!/bin/bash
cat $1 | \
    grep -E '\[.*\]' | \
    awk '{for (i=2; i<NF-1; i++) printf $i " "; print $(NF-1) }; END {print NR,NF-2}' | \
    sed 's/,//g;1h;1d;$!H;$!d;G'
