#include "cu_movegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

unsigned long perft(Board *board, int depth) {
    if (depth == 0)
        return 1;

    Movelist mlist;

    mlist_generate_legal(&mlist, board);

    if (depth == 1)
        return mlist_size(&mlist);

    Boardstack stack;
    unsigned long count = 0;

    for (move_t *iter = mlist_begin(&mlist); iter < mlist_end(&mlist); ++iter) {
        board_push(board, *iter, &stack);
        count += perft(board, depth - 1);
        board_pop(board);
    }

    return count;
}

unsigned long get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Main takes three arguments: the FEN representation of the board,
// the depth of the perft, and either 'bench' for outputting time and node
// results, or an integer for verifying perft correctness.

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <fen> <depth> <bench|N>\n", argv[0]);
        return 1;
    }

    bool bench = !strcmp(argv[3], "bench");

    cu_init();

    Board board;
    Boardstack stack;

    if (board_from_fen(&board, &stack, argv[1])) {
        printf("FAIL: board_from_fen() error: %s\n", argv[1], board_get_error(&board));
        return 1;
    }

    unsigned long start = get_time_ms();
    unsigned long count = perft(&board, atoi(argv[2]));
    unsigned long elapsed = get_time_ms() - start;

    if (!bench) {
        unsigned long expected = strtoul(argv[3], NULL, 10);

        if (count != expected) {
            printf("FAIL: got %lu, expected %lu\n", count, expected);
            return 2;
        }
        else
            printf("OK\n");
    }
    else {
        unsigned long nps = (count * 1000) / (elapsed + !elapsed);
        printf("Nodes: %lu\n", count);
        printf("Time:  %lu.%03lu seconds\n", elapsed / 1000, elapsed % 1000);
        printf("Speed: %lu.%03lu Mnps\n", nps / 1000000, (nps / 1000) % 1000);
    }

    return 0;
}
