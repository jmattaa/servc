#!/usr/bin/env bash

rep=$1
if [ -z "$rep" ]; then
    rep=1
fi

for i in $(seq 1 $rep); do
    curl 127.0.0.1:6969 &
done

wait
