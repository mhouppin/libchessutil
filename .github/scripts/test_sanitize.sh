#!/bin/bash

# Go to repo root directory if needed
cd $(dirname $0)
cd ../..

CRASH_COUNTER=0

# Check which test we are running
name=""

case $1 in
    --asan)
        echo "Running tests under AddressSanitizer..."
        gcc -g3 -fsanitize=address -I include -o perft_check test/perft_check.c libchessutil_asan.a || exit 1 ;;

    --ubsan)
        echo "Running tests under UndefinedBehaviorSanitizer..."
        gcc -g3 -fsanitize=undefined -I include -o perft_check test/perft_check.c libchessutil_ubsan.a || exit 1 ;;

    --bench)
        echo "Running benchmark tests..."
        gcc -O3 -flto -I include -o perft_check test/perft_check.c libchessutil_lto.a || exit 1 ;;

    *)
        exit 1 ;;
esac

exec ./perft_check
