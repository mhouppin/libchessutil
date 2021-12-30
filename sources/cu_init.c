// Libchessutil, a library for chess utilities in C/C++
// Copyright (C) 2021 Morgan Houppin
//
// Libchessutil is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Libchessutil is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <string.h>
#include "cu_core.h"

uint8_t __cu_square_distance[SQUARE_NB][SQUARE_NB];

bitboard_t __cu_line_bb[SQUARE_NB][SQUARE_NB];
bitboard_t __cu_pseudo_moves_bb[PIECETYPE_NB][SQUARE_NB];
bitboard_t __cu_pawn_moves_bb[COLOR_NB][SQUARE_NB];
cu_magic_t __cu_rook_magics[SQUARE_NB];
cu_magic_t __cu_bishop_magics[SQUARE_NB];
bitboard_t __cu_rook_mtable[0x19000];
bitboard_t __cu_bishop_mtable[0x1480];

hashkey_t __cu_zobrist_psq[PIECE_NB][SQUARE_NB];
hashkey_t __cu_zobrist_ep[FILE_NB];
hashkey_t __cu_zobrist_castling[CASTLING_NB];
hashkey_t __cu_zobrist_turn;

const char *cu_get_version(void) {
    return "1.0.2";
}

// Computes the reachable squares from sq given the possible directions and
// occupancy bits. (Note that we allow reaching an occupancy square as if it
// was a capture.)
static bitboard_t __cu_sliding_attack(const direction_t directions[4], square_t sq, bitboard_t occupancy) {
    bitboard_t attack = 0;

    for (int i = 0; i < 4; ++i)
        for (square_t slide = sq + directions[i];
                is_valid_square(slide) && square_distance(slide, slide - directions[i]) == 1;
                slide += directions[i]) {
            attack |= square_bb(slide);
            if (occupancy & square_bb(slide))
                break ;
        }

    return attack;
}

// Initializes the magic bitboards for the given table and directions.
static void __cu_magic_init(bitboard_t *table, cu_magic_t *magics,
                            const direction_t directions[4]) {
    // To avoid compiler warnings.
    int size = 0;

    // The seed for the internal random number generator.
    uint64_t xsSeed = 20650;

    // Declare arrays out of the loop so that compilers don't have to deal
    // with optimizing stack pointer operations.
    bitboard_t occupancy[4096];
    bitboard_t reachable[4096];

    // The epoch is used to determine which iteration of the occupancy test
    // we are in, to avoid zeroing the attack array between each failed
    // iteration.
    int epochTable[4096];
    int currentEpoch = 0;

    memset(epochTable, 0, sizeof(epochTable));

    for (square_t sq = SQ_A1; sq <= SQ_H8; ++sq) {

        // The edges of the board are not counted in the occupancy bits
        // (Since we can reach them whether there's a piece on them or not
        // because of capture moves), but we must still ensure they're
        // accounted for if the piece is already on them for Rook moves.
        bitboard_t edges = ((RANK_1_BB | RANK_8_BB) & ~square_rank_bb(sq))
                         | ((FILE_A_BB | FILE_H_BB) & ~square_file_bb(sq));

        cu_magic_t *mEntry = magics + sq;

        // Compute the occupancy for the given square, excluding edges as
        // explained before.
        mEntry->mask = __cu_sliding_attack(directions, sq, 0) & ~edges;

        // We will need popcount(mask) bits of information for indexing
        // the occupancy, and 1 << popcount(mask) entries in the table
        // for storing the corresponding attack bitboards.
        mEntry->shift = 64 - popcount(mEntry->mask);

        // We use the entry count of the previous square for having the
        // next index.
        mEntry->moves = (sq == SQ_A1) ? table : (mEntry - 1)->moves + size;

        bitboard_t iterBB = 0;
        size = 0;

        // Iterate over all subsets of the occupancy mask with the
        // Carry-Rippler trick and compute all attack bitboards for the current
        // square based on the occupancy.
        do {
            occupancy[size] = iterBB;
            reachable[size] = __cu_sliding_attack(directions, sq, iterBB);
            ++size;
            iterBB = (iterBB - mEntry->mask) & mEntry->mask;
        } while (iterBB);

        int i = 0;

        // Now loop until we find a magic that maps each occupancy to a correct
        // index in our magic table. We optimize the loop by using two binary
        // ANDs to reduce the number of significant bits in the magic
        // (good magics generally have high bit sparsity), and we reduce
        // further the range of tested values by removing magics which do not
        // generate enough significant bits for a full occupancy mask.
        while (i < size) {
            for (mEntry->magic = 0; popcount(mEntry->magic * mEntry->mask >> 56) < 6; ) {
                mEntry->magic = cu_xorshift(&xsSeed);
                mEntry->magic &= cu_xorshift(&xsSeed);
                mEntry->magic &= cu_xorshift(&xsSeed);
            }

            // Check if the generated magic correctly maps each occupancy to
            // its corresponding bitboard attack. Note that we build the table
            // for the square as we test for each occupancy as a speedup, and
            // that we allow two different occupancies to map to the same index
            // if their corresponding bitboard attack is identical.
            for (++currentEpoch, i = 0; i < size; ++i) {
                unsigned int index = magic_index(mEntry, occupancy[i]);

                // Check if we already wrote an attack bitboard at this index
                // during this iteration of the loop. If not, we can set
                // the attack bitboard corresponding to the occupancy at this
                // index; otherwise we check if the attack bitboard already
                // written is identical to the current one, and if it's not
                // the case, the mapping failed, and we must try another value.
                if (epochTable[index] != currentEpoch) {
                    epochTable[index] = currentEpoch;
                    mEntry->moves[index] = reachable[i];
                }
                else if (mEntry->moves[index] != reachable[i])
                    break ;
            }
        }
    }
}

void cu_init(void) {
    static const direction_t __rook_dirs[4] = {NORTH, EAST, SOUTH, WEST};
    static const direction_t __bishop_dirs[4] = {NORTH_EAST, SOUTH_EAST, NORTH_WEST, SOUTH_WEST};
    static const direction_t __knight_dirs[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    static const direction_t __king_dirs[8] = {-9, -8, -7, -1, 1, 7, 8, 9};

    // Zero the line and pseudo_move tables.
    memset(__cu_line_bb, 0, sizeof(__cu_line_bb));
    memset(__cu_pseudo_moves_bb, 0, sizeof(__cu_pseudo_moves_bb));

    // Initialize the cached square_distance() results.
    for (square_t sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (square_t sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
            __cu_square_distance[sq1][sq2] = __cu_max(file_distance(sq1, sq2), rank_distance(sq1, sq2));

    // Initialize the fancy magic bitboard tables.
    __cu_magic_init(__cu_rook_mtable, __cu_rook_magics, __rook_dirs);
    __cu_magic_init(__cu_bishop_mtable, __cu_bishop_magics, __bishop_dirs);

    // Initialize the pseudo-move bitboard tables, along with the line bitboard table.
    for (square_t sq = SQ_A1; sq <= SQ_H8; ++sq) {
        __cu_pawn_moves_bb[WHITE][sq] = pawn_attacks_bb(square_bb(sq), WHITE);
        __cu_pawn_moves_bb[BLACK][sq] = pawn_attacks_bb(square_bb(sq), BLACK);

        for (int i = 0; i < 8; ++i) {
            square_t to = sq + __knight_dirs[i];

            if (is_valid_square(to) && square_distance(sq, to) == 2)
                __cu_pseudo_moves_bb[KNIGHT][sq] |= square_bb(to);

            to = sq + __king_dirs[i];

            if (is_valid_square(to) && square_distance(sq, to) == 1)
                __cu_pseudo_moves_bb[KING][sq] |= square_bb(to);
        }

        __cu_pseudo_moves_bb[QUEEN][sq] = __cu_pseudo_moves_bb[BISHOP][sq] = bishop_moves_bb(sq, 0);
        __cu_pseudo_moves_bb[QUEEN][sq] |= __cu_pseudo_moves_bb[ROOK][sq] = rook_moves_bb(sq, 0);

        for (square_t to = SQ_A1; to <= SQ_H8; ++to) {
            if (__cu_pseudo_moves_bb[BISHOP][sq] & square_bb(to))
                __cu_line_bb[sq][to] = (bishop_moves_bb(sq, 0) & bishop_moves_bb(to, 0))
                    | square_bb(sq) | square_bb(to);

            if (__cu_pseudo_moves_bb[ROOK][sq] & square_bb(to))
                __cu_line_bb[sq][to] = (rook_moves_bb(sq, 0) & rook_moves_bb(to, 0))
                    | square_bb(sq) | square_bb(to);
        }
    }

    // Initialize the Zobrist tables.
    uint64_t state = 0x7F6E5D4C3B2A1908ul;

    // Initialize the Zobrist piece-square table.
    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt) {
            piece_t pc = create_piece(c, pt);

            for (square_t sq = SQ_A1; sq <= SQ_H8; ++sq)
                __cu_zobrist_psq[pc][sq] = cu_xorshift(&state);
        }

    // Initialize the Zobrist en-passant table.
    for (file_t f = FILE_A; f <= FILE_H; ++f)
        __cu_zobrist_ep[f] = cu_xorshift(&state);

    // Initialize the Zobrist castling table.
    for (castling_t castling = NO_CASTLING; castling < CASTLING_NB; ++castling)
        __cu_zobrist_castling[castling] = cu_xorshift(&state);

    // Initialize the Zobrist turn value.
    __cu_zobrist_turn = cu_xorshift(&state);
}
