#!/bin/bash

# Go to repo root directory if needed
cd $(dirname $0)
cd ../..

CRASH_COUNTER=0

# Check which test we are running
name=""

case $1 in
    --asan)
        gcc -g3 -fsanitize=address -I include -o perft_check test/perft_check.c libchessutil_asan.a || exit 1
        name="ASan "
        suffix='2>&1 | grep -A50 "AddressSanitizer:"' ;;

    --ubsan)
        gcc -g3 -fsanitize=undefined -I include -o perft_check test/perft_check.c libchessutil_ubsan.a || exit 1
        name="UbSan"
        suffix='2>&1 | grep -A50 "runtime error:"' ;;

    --bench)
        gcc -O3 -flto -I include -o perft_check test/perft_check.c libchessutil_lto.a || exit 1
        name="Bench"
        suffix='' ;;

    *)
        exit 1 ;;
esac

while read -r line
do
    fen="$(echo $line | cut -d '|' -f1 | xargs)"
    depth="$(echo $line | cut -d '|' -f2 | xargs)"
    nodes="$(echo $line | cut -d '|' -f3 | xargs)"

    if [ $1 = --bench ]; then nodes=bench; fi

    printf "%120s" "Testing fen $fen at depth $depth ($name)... "
    ./perft_check "$fen" $depth $nodes 2>&1 > output.txt

    exit_status=$?

    if [ $exit_status -ne 0 ] && [ $exit_status -ne 1 ]
    then
        echo "CRASH (see 'crash_log.txt' for more info)"
        cat output.txt >> crash_log.txt
        let "CRASH_COUNTER+=1"
        if [ $CRASH_COUNTER -eq 3 ]
        then
            echo "Aborting tests, too much crashes."
            exit
        fi
    else
        cat output.txt | tail -n 1
    fi
done < datasets/perft.txt

rm -f perft_check
rm -f output.txt