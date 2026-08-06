#!/usr/bin/env bash

target=howldb
bin=./howlingdb
if [[ $1 == -t ]]; then
    target=page_test
    bin=./page_test
fi

while true; do
    clear
    make -j "$target" && "$bin"
    inotifywait -qq -r src tests main.cpp -e modify -e move
done

./watcher.sh → builds/runs howldb; ./watcher.sh -t → page_test.
