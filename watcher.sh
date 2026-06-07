#!/usr/bin/env bash

while true;
do
    echo "==============================================="
    make -j
    ./howlingdb
    inotifywait -r . -e MODIFY -e MOVE
done
