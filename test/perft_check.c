#include "cu_movegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


const char *PERFT_LIST[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 | 20 400 8902 197281 4865609 119060324",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 | 48 2039 97862 4085603 193690690",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 | 14 191 2812 43238 674624 11030083 178633661",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1 | 6 264 9467 422333 15833292",
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 | 6 264 9467 422333 15833292",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 | 44 1486 62379 2103487 89941194",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 | 46 2079 89890 3894594 164075551",

    "nqnbrkbr/1ppppp1p/p7/6p1/6P1/P6P/1PPPPP2/NQNBRKBR w HEhe - 1 9 | 20 382 8694 187263 4708975 112278808",
    "nnbrkbrq/1pppp1p1/p7/7p/1P2Pp2/BN6/P1PP1PPP/1N1RKBRQ w GDgd - 0 9 | 27 482 13441 282259 8084701 193484216",
    "nrbnkrqb/pppp1p1p/4p1p1/8/7P/2P1P3/PPNP1PP1/1RBNKRQB w FBfb - 0 9 | 20 459 9998 242762 5760165 146614723",
    "qnrbb1nr/pp1p1ppp/2p2k2/4p3/4P3/5PPP/PPPP4/QNRBBKNR w HC - 0 9 | 20 460 10287 241640 5846781 140714047",
    "1qnnbrkb/rppp1ppp/p3p3/8/4P3/2PP1P2/PP4PP/RQNNBKRB w GA - 1 9 | 24 479 12135 271469 7204345 175460841",
    NULL
};

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

int main(void) {
    cu_init();

    Board board;
    Boardstack stack;

    size_t testCount;
    for (testCount = 0; PERFT_LIST[testCount] != NULL; ++testCount);

    unsigned long start = get_time_ms();
    unsigned long nodes = 0;

    size_t i;
    for (i = 0; i < testCount; ++i) {
        printf("Running perft test %lu/%lu... ", (unsigned long)i + 1, (unsigned long)testCount);
        fflush(stdout);

        if (board_from_fen(&board, &stack, PERFT_LIST[i])) {
            printf("FAIL: board_from_fen() error: %s\n", board_get_error(&board));
            return 1;
        }

        char *ptr = strchr(PERFT_LIST[i], '|') + 1;
        int depth = 0;

        while (*ptr) {
            char *endPtr;
            unsigned long expected = strtoul(ptr, &endPtr, 10);
            unsigned long count;

            ++depth;
            count = perft(&board, depth);
            nodes += count;

            if (count != expected) {
                int fenLength = strcspn(PERFT_LIST[i], "|") - 1;
                printf("\nFail for FEN '%.*s' at depth %d: expected %lu, got %lu\n",
                    fenLength, PERFT_LIST[i], depth, expected, count);
                fflush(stdout);
                break ;
            }
            ptr = endPtr + strspn(endPtr, " \t");
        }

        if (!*ptr) {
            puts("OK");
            fflush(stdout);
        }
    }


    unsigned long elapsed = get_time_ms() - start;

    unsigned long nps = (nodes * 1000) / (elapsed + !elapsed);
    printf("Nodes: %lu\n", nodes);
    printf("Time:  %lu.%03lu seconds\n", elapsed / 1000, elapsed % 1000);
    printf("Speed: %lu.%03lu Mnps\n", nps / 1000000, (nps / 1000) % 1000);

    return 0;
}
