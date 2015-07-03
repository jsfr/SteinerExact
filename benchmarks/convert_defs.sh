#!/bin/zsh

from=$1
to=$2

for instance in `ls $from`; do
    rm -rf $to/$instance
    mkdir $to/$instance
    for file in `ls $from/$instance`; do
        {
            echo '{\n    "points": [';
            grep -E "^(D)+ [ \.0-9\-]*$" $from/$instance/$file | \
                cut -d " " -f 3- | \
                sed -e 's/\(\w\) /\1, /g;s/, $//;s/^.*$/\        [ \0 \],/;$s/,$//';
            echo '    ]\n}';
        } > $to/$instance/${file/stp/json}
    done
done
