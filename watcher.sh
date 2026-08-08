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
    inotifywait -qq -r . @.git -e modify -e move \
        --exclude '(^|/)(page_test|howlingdb)$|\.(o|out)$'
done

./watcher.sh → builds/runs howldb; ./watcher.sh -t → page_test.
