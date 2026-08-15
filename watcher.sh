#!/usr/bin/env bash
# Rebuilds and runs both howldb and page_test on any save in the repo.

targets=(howldb page_test)
bins=(./howlingdb ./page_test)

while true; do
    clear
    for i in "${!targets[@]}"; do
        echo "=============== ${targets[$i]} ==============="
        make -j "${targets[$i]}" && "${bins[$i]}"
    done
    inotifywait -qq -r . @.git -e modify -e move \
        --exclude '(^|/)(page_test|howlingdb)$|\.(o|out)$'
done
