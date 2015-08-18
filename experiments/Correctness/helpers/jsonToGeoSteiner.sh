#!/bin/bash
cat $1 | grep -E '\[.*\]' | awk '{print $2 $3}' | sed 's/,/ /g'
