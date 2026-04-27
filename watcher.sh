#!/usr/bin/env bash

while true;
do
    echo "==============================================="
    make -j
    ./dbms
    inotifywait -r . -e MODIFY -e MOVE
done
