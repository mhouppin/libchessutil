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

#ifndef __CU_CORE_H__
#define __CU_CORE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// If used inside C++ code, inform the compiler that the functions are in C.
#if defined(__cplusplus)
#define __CU_BEGIN_DECLS extern "C" {
#define __CU_END_DECLS }
#else
#define __CU_BEGIN_DECLS
#define __CU_END_DECLS
#endif

// Use __builtin_() functions if we are allowed to.
// Note: if you find build issues with builtins enabled, add "-DCU_NO_BUILTINS"
// to the CFLAGS variable.
#ifdef CU_NO_BUILTINS
#define CU_USE_BUILTINS 0
#elif defined(__GNUC__)
#define CU_USE_BUILTINS 1
#else
#define CU_USE_BUILTINS 0
#endif

#if CU_USE_BUILTINS

#if (UINT_MAX == UINT64_MAX)
#define __cu_popcount(x) __builtin_popcount(x)
#define __cu_tzcnt(x)    __builtin_ctz(x)
#define __cu_lzcnt(x)    __builtin_clz(x)
#elif (ULONG_MAX == UINT64_MAX)
#define __cu_popcount(x) __builtin_popcountl(x)
#define __cu_tzcnt(x)    __builtin_ctzl(x)
#define __cu_lzcnt(x)    __builtin_clzl(x)
#elif (ULONG_MAX == UINT32_MAX)
#define __cu_popcount(x) (__builtin_popcountl((uint32_t)x) + __builtin_popcountl(x >> 32))
#define __cu_tzcnt(x)    (!(uint32_t)x ? __builtin_ctzl(x >> 32) + 32 : __builtin_ctzl((uint32_t)x))
#define __cu_lzcnt(x)    (!(x >> 32) ? __builtin_clzl((uint32_t)x) + 32 : __builtin_clzl(x >> 32))
#endif

#endif // CU_USE_BUILTINS

#define __CU_INLINE static inline

#define CU_MAX_MOVES 512

__CU_BEGIN_DECLS

// Returns the version string of the library.
// Version will be in the format "x.y.z".
const char *cu_get_version(void);

// Initializes all stuff related to the library. It should be called before any
// other function of the library (except cu_get_version()).
void cu_init(void);

// Internal max() helper for the library.
__CU_INLINE int __cu_max(int a, int b) {
    return a > b ? a : b;
}

// Internal min() helper for the library.
__CU_INLINE int __cu_min(int a, int b) {
    return a < b ? a : b;
}

// Internal abs() helper for the library.
__CU_INLINE int __cu_abs(int x) {
    return x < 0 ? -x : x;
}

// Typedef for color.
typedef uint8_t color_t;

// Enum for color values.
enum color_e {
    WHITE, BLACK, COLOR_NB = 2
};

// Flips the given color.
__CU_INLINE color_t flip_color(color_t c) {
    return c ^ 1;
}

// Typedef for piece type.
typedef uint8_t piecetype_t;

// Enum for piece type values.
enum piecetype_e {
    NO_PIECETYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES = 0, PIECETYPE_NB = 8
};

// Typedef for piece.
typedef uint8_t piece_t;

// Enum for piece values.
enum piece_e {
    NO_PIECE,
    WHITE_PAWN = 1, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWN = 9, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    PIECE_NB = 16
};

// Creates a piece given the color and piece type.
__CU_INLINE piece_t create_piece(color_t c, piecetype_t pt) {
    return (c << 3) | pt;
}

// Returns the type of the given piece.
__CU_INLINE piecetype_t piece_type(piece_t pc) {
    return pc & 7;
}

// Returns the color of the given piece.
__CU_INLINE color_t piece_color(piece_t pc) {
    return pc >> 3;
}

// Flips the color of the piece.
__CU_INLINE piece_t flip_piece(piece_t pc) {
    return pc ^ 8;
}

// Typedef for file.
typedef uint8_t file_t;

// Enum for file values.
enum file_e {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB = 8
};

// Typedef for rank.
typedef uint8_t rank_t;

// Enum for rank values.
enum rank_e {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB = 8
};

// Typedef for square.
typedef uint8_t square_t;

// Enum for square values.
enum square_e {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE, SQUARE_NB = 64
};

// Typedef for direction.
typedef uint8_t direction_t;

// Enum for direction values.
enum direction_e {
    NORTH = 8, EAST = 1, SOUTH = -8, WEST = -1,
    NORTH_EAST = NORTH + EAST, SOUTH_EAST = SOUTH + EAST,
    NORTH_WEST = NORTH + WEST, SOUTH_WEST = SOUTH + WEST,
};

// Internal table for square_distance() function.
extern uint8_t __cu_square_distance[SQUARE_NB][SQUARE_NB];

// Creates a square given the file and the rank.
__CU_INLINE square_t create_square(file_t f, rank_t r) {
    return (r << 3) | f;
}

// Checks if the square is valid.
__CU_INLINE bool is_valid_square(square_t sq) {
    return sq < SQUARE_NB;
}

// Returns the file of the square.
__CU_INLINE file_t square_file(square_t sq) {
    return sq & 7;
}

// Returns the rank of the square.
__CU_INLINE rank_t square_rank(square_t sq) {
    return sq >> 3;
}

// Flips the square file.
__CU_INLINE square_t flip_square_file(square_t sq) {
    return sq ^ 7;
}

// Flips the square rank.
__CU_INLINE square_t flip_square_rank(square_t sq) {
    return sq ^ 56;
}

// Returns the square relative to the color's POV.
__CU_INLINE square_t relative_square(square_t sq, color_t c) {
    return sq ^ (c * 56);
}

// Returns the rank relative to the color's POV.
__CU_INLINE rank_t relative_rank(rank_t r, color_t c) {
    return r ^ (c * 7);
}

// Returns the square rank relative to the color's POV.
__CU_INLINE rank_t relative_square_rank(square_t sq, color_t c) {
    return relative_rank(square_rank(sq), c);
}

// Returns the direction of pawn pushes for the given color.
__CU_INLINE direction_t pawn_direction(color_t c) {
    return c == WHITE ? NORTH : SOUTH;
}

// Returns the file distance between two squares.
__CU_INLINE int file_distance(square_t sq1, square_t sq2) {
    return __cu_abs((int8_t)(square_file(sq1) - square_file(sq2)));
}

// Returns the rank distance between two squares.
__CU_INLINE int rank_distance(square_t sq1, square_t sq2) {
    return __cu_abs((int8_t)(square_rank(sq1) - square_rank(sq2)));
}

// Returns the King distance between two squares.
__CU_INLINE int square_distance(square_t sq1, square_t sq2) {
    return __cu_square_distance[sq1][sq2];
}

// Typedef for move.
typedef uint16_t move_t;

// Enum for move values.
enum move_e {
    NO_MOVE, NULL_MOVE = 65
};

// Typedef for move type.
typedef uint16_t movetype_t;

// Enum for move type values.
enum movetype_e {
    NORMAL_MOVE,
    PROMOTION     = 1 << 14,
    EN_PASSANT    = 2 << 14,
    CASTLING      = 3 << 14,
    MOVETYPE_MASK = 3 << 14
};

// Creates a move given the source and destination squares, and the move type.
__CU_INLINE move_t create_move(square_t from, square_t to, movetype_t mt) {
    return (from << 6) | to | mt;
}

// Creates a promotion move given the source and destination squares, and the
// promotion type.
__CU_INLINE move_t create_promotion(square_t from, square_t to, piecetype_t pt) {
    return ((pt - KNIGHT) << 12) | create_move(from, to, PROMOTION);
}

// Returns the type of the move.
__CU_INLINE movetype_t move_type(move_t m) {
    return m & MOVETYPE_MASK;
}

// Returns the promotion type of the move.
__CU_INLINE piecetype_t promotion_type(move_t m) {
    return ((m >> 12) & 3) + KNIGHT;
}

// Returns the source square of the move.
__CU_INLINE square_t move_from(move_t m) {
    return (m >> 6) & 63;
}

// Returns the destination square of the move.
__CU_INLINE square_t move_to(move_t m) {
    return m & 63;
}

// Returns the square mask of the move (it effectively masks the move type and
// the promotion type).
__CU_INLINE uint16_t move_square_mask(move_t m) {
    return m & 0xFFF;
}

// Checks if the move is 'valid' (that is, different source and destination
// squares). Please note that by this definition, a nullmove isn't considered
// as a valid move (for example, move_is_pseudo_legal() will always return
// false for a nullmove).
__CU_INLINE bool is_valid_move(move_t m) {
    return move_from(m) != move_to(m);
}

// Typedef for castling rights.
typedef uint8_t castling_t;

// Enum for castling rights.
enum castling_e {
    NO_CASTLING,
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,

    WHITE_CASTLING     = WHITE_OO  | WHITE_OOO,
    BLACK_CASTLING     = BLACK_OO  | BLACK_OOO,
    KINGSIDE_CASTLING  = WHITE_OO  | BLACK_OO,
    QUEENSIDE_CASTLING = WHITE_OOO | BLACK_OOO,

    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING,
    CASTLING_NB  = 16
};

// Returns the mask of castlings for the given color.
__CU_INLINE castling_t castling_color_mask(color_t c) {
    return c == WHITE ? WHITE_CASTLING : BLACK_CASTLING;
}

// Typedef for bitboard.
typedef uint64_t bitboard_t;

// Macros for bitboard values.

#define FILE_A_BB UINT64_C(0x0101010101010101)
#define FILE_B_BB UINT64_C(0x0202020202020202)
#define FILE_C_BB UINT64_C(0x0404040404040404)
#define FILE_D_BB UINT64_C(0x0808080808080808)
#define FILE_E_BB UINT64_C(0x1010101010101010)
#define FILE_F_BB UINT64_C(0x2020202020202020)
#define FILE_G_BB UINT64_C(0x4040404040404040)
#define FILE_H_BB UINT64_C(0x8080808080808080)
#define RANK_1_BB UINT64_C(0x00000000000000FF)
#define RANK_2_BB UINT64_C(0x000000000000FF00)
#define RANK_3_BB UINT64_C(0x0000000000FF0000)
#define RANK_4_BB UINT64_C(0x00000000FF000000)
#define RANK_5_BB UINT64_C(0x000000FF00000000)
#define RANK_6_BB UINT64_C(0x0000FF0000000000)
#define RANK_7_BB UINT64_C(0x00FF000000000000)
#define RANK_8_BB UINT64_C(0xFF00000000000000)

#define KINGSIDE_BB      UINT64_C(0xF0F0F0F0F0F0F0F0)
#define QUEENSIDE_BB     UINT64_C(0x0F0F0F0F0F0F0F0F)
#define LIGHT_SQUARES_BB UINT64_C(0x55AA55AA55AA55AA)
#define DARK_SQUARES_BB  UINT64_C(0xAA55AA55AA55AA55)
#define ALL_SQUARES_BB   UINT64_C(0xFFFFFFFFFFFFFFFF)

// Data structure used in fancy magic bitboards.
typedef struct cu_magic_s {
    bitboard_t mask;
    bitboard_t magic;
    bitboard_t *moves;
    unsigned int shift;
} cu_magic_t;

// Internal tables for bitboard operations like pseudo-move generation and
// square alignment checks.
extern bitboard_t __cu_line_bb[SQUARE_NB][SQUARE_NB];
extern bitboard_t __cu_pseudo_moves_bb[PIECETYPE_NB][SQUARE_NB];
extern bitboard_t __cu_pawn_moves_bb[COLOR_NB][SQUARE_NB];
extern cu_magic_t __cu_rook_magics[SQUARE_NB];
extern cu_magic_t __cu_bishop_magics[SQUARE_NB];

// Returns the index of the attack bitboard for the given magic and occupancy.
__CU_INLINE unsigned int magic_index(const cu_magic_t *magic, bitboard_t occupancy) {
    return (unsigned int)(((occupancy & magic->mask) * magic->magic) >> magic->shift);
}

// Returns the bitboard representation of the square.
__CU_INLINE bitboard_t square_bb(square_t sq) {
    return (bitboard_t)1 << sq;
}

// Shifts a bitboard to the north.
__CU_INLINE bitboard_t bb_shift_north(bitboard_t bb) {
    return bb << 8;
}

// Shifts a bitboard to the east.
__CU_INLINE bitboard_t bb_shift_east(bitboard_t bb) {
    return (bb & ~FILE_H_BB) << 1;
}

// Shifts a bitboard to the south.
__CU_INLINE bitboard_t bb_shift_south(bitboard_t bb) {
    return bb >> 8;
}

// Shifts a bitboard to the west.
__CU_INLINE bitboard_t bb_shift_west(bitboard_t bb) {
    return (bb & ~FILE_A_BB) >> 1;
}

// Shifts a bitboard to the north-east.
__CU_INLINE bitboard_t bb_shift_north_east(bitboard_t bb) {
    return (bb & ~FILE_H_BB) << 9;
}

// Shifts a bitboard to the south-east.
__CU_INLINE bitboard_t bb_shift_south_east(bitboard_t bb) {
    return (bb & ~FILE_H_BB) >> 7;
}

// Shifts a bitboard to the south-west.
__CU_INLINE bitboard_t bb_shift_south_west(bitboard_t bb) {
    return (bb & ~FILE_A_BB) >> 9;
}

// Shifts a bitboard to the north-west.
__CU_INLINE bitboard_t bb_shift_north_west(bitboard_t bb) {
    return (bb & ~FILE_A_BB) << 7;
}

// Shifts a bitboard to the relative north of the given color.
__CU_INLINE bitboard_t bb_relative_shift_north(bitboard_t bb, color_t c) {
    return c == WHITE ? bb_shift_north(bb) : bb_shift_south(bb);
}

// Shifts a bitboard to the relative south of the given color.
__CU_INLINE bitboard_t bb_relative_shift_south(bitboard_t bb, color_t c) {
    return c == WHITE ? bb_shift_south(bb) : bb_shift_north(bb);
}

// Checks if the bitboard has two or more squares set.
__CU_INLINE bool more_than_one_bit(bitboard_t bb) {
    return bb & (bb - 1);
}

// Returns the bitboard representation of the file.
__CU_INLINE bitboard_t file_bb(file_t f) {
    return (bitboard_t)FILE_A_BB << f;
}

// Returns the bitboard representation of the square file.
__CU_INLINE bitboard_t square_file_bb(square_t sq) {
    return file_bb(square_file(sq));
}

// Returns the bitboard representation of the rank.
__CU_INLINE bitboard_t rank_bb(rank_t r) {
    return (bitboard_t)RANK_1_BB << (r * 8);
}

// Returns the bitboard representation of the square rank.
__CU_INLINE bitboard_t square_rank_bb(square_t sq) {
    return rank_bb(square_rank(sq));
}

// Returns the bitboard of all squares between two squares, excluding them.
__CU_INLINE bitboard_t between_squares_bb(square_t sq1, square_t sq2) {
    return __cu_line_bb[sq1][sq2] & (
        (ALL_SQUARES_BB << (sq1 +  (sq1 < sq2)))
      ^ (ALL_SQUARES_BB << (sq2 + !(sq1 < sq2)))
    );
}

// Checks if three squares are aligned on the board along a file, rank or
// diagonal.
__CU_INLINE bool squares_aligned(square_t sq1, square_t sq2, square_t sq3) {
    return __cu_line_bb[sq1][sq2] & square_bb(sq3);
}

// Returns the bitboard of Pawn attacks for the given square and color.
__CU_INLINE bitboard_t pawn_moves_bb(square_t sq, color_t c) {
    return __cu_pawn_moves_bb[c][sq];
}

// Returns the bitboard of Knight moves for the given square.
__CU_INLINE bitboard_t knight_moves_bb(square_t sq) {
    return __cu_pseudo_moves_bb[KNIGHT][sq];
}

// Returns the bitboard of Bishop moves for the given square and occupancy.
__CU_INLINE bitboard_t bishop_moves_bb(square_t sq, bitboard_t occupancy) {
    const cu_magic_t *magic = &__cu_bishop_magics[sq];
    return magic->moves[magic_index(magic, occupancy)];
}

// Returns the bitboard of Rook moves for the given square and occupancy.
__CU_INLINE bitboard_t rook_moves_bb(square_t sq, bitboard_t occupancy) {
    const cu_magic_t *magic = &__cu_rook_magics[sq];
    return magic->moves[magic_index(magic, occupancy)];
}

// Returns the bitboard of Queen moves for the given square and occupancy.
__CU_INLINE bitboard_t queen_moves_bb(square_t sq, bitboard_t occupancy) {
    return bishop_moves_bb(sq, occupancy) | rook_moves_bb(sq, occupancy);
}

// Returns the bitboard of King moves for the given square.
__CU_INLINE bitboard_t king_moves_bb(square_t sq) {
    return __cu_pseudo_moves_bb[KING][sq];
}

// Returns the bitboard of moves for the given piece type, square and occupancy.
// If the piece type is empty or invalid, returns zero.
__CU_INLINE bitboard_t attacks_bb(piecetype_t pt, square_t sq, bitboard_t occupancy) {
    switch (pt) {
        case KNIGHT: return knight_moves_bb(sq);
        case BISHOP: return bishop_moves_bb(sq, occupancy);
        case ROOK:   return rook_moves_bb(sq, occupancy);
        case QUEEN:  return queen_moves_bb(sq, occupancy);
        case KING:   return king_moves_bb(sq);
        default:     return 0;
    }
}

// Returns the bitboard of Pawn attacks for the given bitboard and color.
__CU_INLINE bitboard_t pawn_attacks_bb(bitboard_t bb, color_t c) {
    bb = bb_relative_shift_north(bb, c);
    return bb_shift_west(bb) | bb_shift_east(bb);
}

// Returns the bitboard of Pawn double attacks for the given bitboard and
// color.
__CU_INLINE bitboard_t pawn_2attacks_bb(bitboard_t bb, color_t c) {
    bb = bb_relative_shift_north(bb, c);
    return bb_shift_west(bb) & bb_shift_east(bb);
}

// Returns the bitboard of the files adjacent to the given square.
__CU_INLINE bitboard_t adjacent_files_bb(square_t sq) {
    bitboard_t fileBB = square_file_bb(sq);
    return bb_shift_west(fileBB) | bb_shift_east(fileBB);
}

// Returns the bitboard of the forward ranks of the given square, relative to
// the given color.
__CU_INLINE bitboard_t forward_ranks_bb(square_t sq, color_t c) {
    return c == WHITE
        ? ~(bitboard_t)RANK_1_BB << 8 * square_rank(sq)
        : ~(bitboard_t)RANK_8_BB >> 8 * (RANK_8 - square_rank(sq));
}

// Returns the bitboard of the forward file of the given square, relative to
// the given color.
__CU_INLINE bitboard_t forward_file_bb(square_t sq, color_t c) {
    return forward_ranks_bb(sq, c) & square_file_bb(sq);
}

// Returns the bitboard of all potential future Pawn attacks from the given
// square and color.
__CU_INLINE bitboard_t pawn_attack_span_bb(square_t sq, color_t c) {
    return forward_ranks_bb(sq, c) & adjacent_files_bb(sq);
}

// Returns the bitboard of all squares controlling the queening path of the
// Pawn.
__CU_INLINE bitboard_t passed_pawn_span_bb(square_t sq, color_t c) {
    return forward_ranks_bb(sq, c) & (adjacent_files_bb(sq) | square_file_bb(sq));
}

// Returns the number of set squares in the bitboard.
__CU_INLINE int popcount(bitboard_t bb) {
#ifdef __cu_popcount
    return __cu_popcount(bb);
#else
    const bitboard_t m1 = 0x5555555555555555ul;
    const bitboard_t m2 = 0x3333333333333333ul;
    const bitboard_t m4 = 0x0F0F0F0F0F0F0F0Ful;
    const bitboard_t hx = 0x0101010101010101ul;

    bb -= ((bb >> 1) & m1);
    bb = (bb & m2) + ((bb >> 2) & m2);
    bb = (bb + (bb >> 4)) & m4;
    return ((bb * hx) >> 56);
#endif
}

// TODO: it would be a good idea to use conditional assembly code for using
// the 'bsf' instruction in the two nex functions when the compiler doesn't
// support GNU extensions. The "builtin-less" code is likely very slow, but
// we probably cannot do much better without using cache tables for
// intermediate 16-bit results.

// Returns the first set square of the given bitboard.
__CU_INLINE square_t bb_first_square(bitboard_t bb) {
#ifdef __cu_tzcnt
    return __cu_tzcnt(bb);
#else
    bitboard_t bitmask = 0xFFFFFFFFul;
    uint8_t count = 32;
    square_t result = 0;

    while (count) {
        if (!(bb & bitmask)) {
            bb >>= count;
            result += count;
        }
        count >>= 1;
        bitmask >>= count;
    }

    return result;
#endif
}

// Returns the last set square of the given bitboard.
__CU_INLINE square_t bb_last_square(bitboard_t bb) {
#ifdef __cu_lzcnt
    return 63 - __cu_lzcnt(bb);
#else
    bitboard_t bitmask = 0xFFFFFFFF00000000ul;
    uint8_t count = 32;
    square_t result = 63;

    while (count) {
        if (!(bb & bitmask)) {
            bb <<= count;
            result -= count;
        }
        count >>= 1;
        bitmask <<= count;
    }

    return result;
#endif
}

// Pops the first set square of the given bitboard and returns it.
__CU_INLINE square_t bb_pop_first_square(bitboard_t *bb) {
    square_t sq = bb_first_square(*bb);
    *bb &= *bb - 1;
    return sq;
}

// Returns the first set square of the given bitboard, relative to the given
// color.
__CU_INLINE square_t bb_relative_first_square(bitboard_t bb, color_t c) {
    return c == WHITE ? bb_first_square(bb) : bb_last_square(bb);
}

// Returns the last set square of the given bitboard, relative to the given
// color.
__CU_INLINE square_t bb_relative_last_square(bitboard_t bb, color_t c) {
    return c == WHITE ? bb_last_square(bb) : bb_first_square(bb);
}

// Using the given state, computes a random number based on a Xorshift
// generator and returns it, updating the state acordingly.
__CU_INLINE uint64_t cu_xorshift(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return *state = x;
}

// Typedef for hashing keys.
typedef uint64_t hashkey_t;

// Internal tables for Zobrist hashing.
extern hashkey_t __cu_zobrist_psq[PIECE_NB][SQUARE_NB];
extern hashkey_t __cu_zobrist_ep[FILE_NB];
extern hashkey_t __cu_zobrist_castling[CASTLING_NB];
extern hashkey_t __cu_zobrist_turn;

// Enum for game outcomes.
typedef enum outcome_e {
    NO_OUTCOME,
    WHITE_WINS,
    BLACK_WINS,
    DRAWN_GAME
} outcome_t;

// Structure for keeping track of the moves played on the board.
typedef struct Boardstack_ {
    struct Boardstack_ *prev;
    hashkey_t key;
    hashkey_t materialKey;
    int rule50;
    int lastNullmove;
    int repetition;
    move_t lastMove;
    square_t enPassantSq;
    square_t polyglotEP;
    castling_t castlingRights;
    piece_t capturedPiece;
    bitboard_t checkers;
    bitboard_t checkBlockers[COLOR_NB];
    bitboard_t checkPinners[COLOR_NB];
    bitboard_t checkSquares[PIECETYPE_NB];
} Boardstack;

// Structure for the board
typedef struct Board_ {
    piece_t table[SQUARE_NB];
    bitboard_t piecetypeBBs[PIECETYPE_NB];
    bitboard_t colorBBs[PIECETYPE_NB];
    int pieceCounts[PIECE_NB];
    int castlingMasks[SQUARE_NB];
    square_t castlingRookSquare[CASTLING_NB];
    bitboard_t castlingPaths[CASTLING_NB];
    char err[128];
    int gamePly;
    color_t sideToMove;
    Boardstack *stack;
    bool chess960;
    bool internalStackAllocator;
} Board;

// Constant for the starting board.
extern const char *const STARTING_FEN;
// Constant for the piece characters.
extern const char PIECE_INDEXES[PIECE_NB];

// Initializes the board from the given FEN.
// The function ensures that the position resulting from the given FEN
// is valid, and will return an error otherwise.
// If stack is NULL, all the stacks will be allocated internally.
// Returns 0 if successful, a non-null value otherwise.
int board_from_fen(Board *board, Boardstack *stack, const char *fen);

// Destroys the given board. Effectively frees all stacks from the board if
// they have been allocated internally, and frees the error string, if any.
void board_destroy(Board *board);

// Resets the board to its initial state. Will free all stacks from the board
// if they have been allocated internally (except for the first one).
void board_reset(Board *board);

// Returns the error string, if any.
__CU_INLINE const char *board_get_error(Board *board) {
    return board->err[0] ? board->err : NULL;
}

// Sets the error string to the given string. If the string is NULL, frees
// the error string.
void board_set_error(Board *board, const char *str);

// Copies the root position of the source board to the destination board.
// If stack is NULL, all the stacks of the destination board will be allocated
// internally. If any error arises, it will be stored in the destination board.
// Returns 0 if successful, a non-null value otherwise.
int board_copy_root(Board *dst, const Board *src, Boardstack *stack);

// Copies the source board to the destination board.
// All the stacks of the destination board will be allocated internally. If any
// error arises, it will be stored in the destination board.
// Returns 0 if successful, a non-null value otherwise.
int board_copy(Board *dst, const Board *src);

// Returns the piece at the given square.
__CU_INLINE piece_t board_piece_at(const Board *board, square_t sq) {
    return board->table[sq];
}

// Tests if the given square is empty.
__CU_INLINE bool board_is_empty(const Board *board, square_t sq) {
    return board_piece_at(board, sq) == NO_PIECE;
}

__CU_INLINE color_t board_turn(const Board *board) {
    return board->sideToMove;
}

// Returns the number of half-moves since the start of the game.
__CU_INLINE int board_ply(const Board *board) {
    return board->gamePly;
}

// Returns the current move number of the game.
__CU_INLINE int board_move_number(const Board *board) {
    return 1 + (board->gamePly - (board_turn(board) == BLACK)) / 2;
}

// Returns the number of moves since the last irreversible move.
__CU_INLINE int board_rule50(const Board *board) {
    return board->stack->rule50;
}

// Checks if the current board is a chess960 position.
__CU_INLINE bool board_is_chess960(const Board *board) {
    return board->chess960;
}

// Returns the bitboard of pieces currently giving check.
__CU_INLINE bitboard_t board_checkers(const Board *board) {
    return board->stack->checkers;
}

// Tests if the side to move is in check.
__CU_INLINE bool board_is_in_check(const Board *board) {
    return !!board_checkers(board);
}

// Returns a bitboard of pieces of the given piece type.
__CU_INLINE bitboard_t board_piecetype_bb(const Board *board, piecetype_t pt) {
    return board->piecetypeBBs[pt];
}

// Returns a bitboard of pieces of the given piece types.
__CU_INLINE bitboard_t board_piecetypes_bb(const Board *board, piecetype_t pt1, piecetype_t pt2) {
    return board->piecetypeBBs[pt1] | board->piecetypeBBs[pt2];
}

// Returns a bitboard of all pieces of the given color.
__CU_INLINE bitboard_t board_color_bb(const Board *board, color_t c) {
    return board->colorBBs[c];
}

// Returns a bitboard of pieces of the given color and piece type.
__CU_INLINE bitboard_t board_piece_bb(const Board *board, color_t c, piecetype_t pt) {
    return board_color_bb(board, c) & board_piecetype_bb(board, pt);
}

// Returns a bitboard of pieces of the given color and piece types.
__CU_INLINE bitboard_t board_pieces_bb(const Board *board, color_t c, piecetype_t pt1, piecetype_t pt2) {
    return board_color_bb(board, c) & board_piecetypes_bb(board, pt1, pt2);
}

// Returns a bitboard of all non-empty squares.
__CU_INLINE bitboard_t board_occupancy_bb(const Board *board) {
    return board->piecetypeBBs[ALL_PIECES];
}

// Returns the number of pieces present on the board.
__CU_INLINE int board_count_piece(const Board *board, piece_t pc) {
    return board->pieceCounts[pc];
}

// Returns the number of pieces of the given piece type present on the board.
__CU_INLINE int board_count_piecetype(const Board *board, piecetype_t pt) {
    return board_count_piece(board, create_piece(WHITE, pt))
         + board_count_piece(board, create_piece(BLACK, pt));
}

// Returns the number of pieces of the given color present on the board.
__CU_INLINE int board_count_color(const Board *board, color_t c) {
    return board_count_piece(board, create_piece(c, ALL_PIECES));
}

// Returns the total number of pieces present on the board.
__CU_INLINE int board_count_all(const Board *board) {
    return board_count_color(board, WHITE) + board_count_color(board, BLACK);
}

// Returns the Zobrist key of the board.
__CU_INLINE hashkey_t board_key(const Board *board) {
    return board->stack->key;
}

// Returns the material key of the board.
__CU_INLINE hashkey_t board_material_key(const Board *board) {
    return board->stack->materialKey;
}

// Returns the square of the piece of the given piecetype and color.
// If several pieces match the given parameters, the one with the lower square
// value is returned.
__CU_INLINE square_t board_piece_square(const Board *board, color_t c, piecetype_t pt) {
    return bb_first_square(board_piece_bb(board, c, pt));
}

// Returns the square of King of the given color.
__CU_INLINE square_t board_king_square(const Board *board, color_t c) {
    return board_piece_square(board, c, KING);
}

// Tests if the given move is pseudo-legal.
bool board_move_is_pseudo_legal(const Board *board, move_t move);

// Tests if the given pseudo-legal move is legal.
bool board_move_is_legal(const Board *board, move_t move);

// Tests if the given move would put the opponent in check. The move must be at
// least pseudo-legal.
bool board_move_gives_check(const Board *board, move_t move);

// Tests if the side to move is checkmated.
bool board_is_checkmate(const Board *board);

// Tests if the side to move is stalemated.
bool board_is_stalemate(const Board *board);

// Tests if the game is drawn by insufficient material.
bool board_is_material_draw(const Board *board);

// Tests if the game is drawn by seventy-five moves rule.
bool board_is_rule75_draw(const Board *board);

// Tests if the game can be drawn by fifty moves rule.
// (For now, this function doesn't check if the side to move can play a move
// and then claim the draw, this might be added as a second parameter in the
// future.)
bool board_is_rule50_draw(const Board *board);

// Tests if the game is drawn by fivefold repetition.
__CU_INLINE bool board_is_fivefold_draw(const Board *board) {
    return board->stack->repetition >= 5;
}

// Tests if the game can be drawn by threefold repetition.
// (For now, this function doesn't check if the side to move can play a move
// and then claim the draw, this might be added as a second parameter in the
// future.)
__CU_INLINE bool board_is_threefold_draw(const Board *board) {
    return board->stack->repetition >= 3;
}

// Returns the outcome of the board, or NO_OUTCOME if the game should continue.
// If claimDraw is set, additionally checks for possible draw claims by the
// fifty moves rule or the threefold repetition.
outcome_t board_outcome(const Board *board, bool claimDraw);

// Updates the board with the given move.
// Returns zero if successful, and a non-null value otherwise.
// (Note that it's not necessary to check for the return value if the board
// is not using an internal stack allocator.)
// Moves are not checked for legality. It is the caller’s responsibility to
// ensure that the move is fully legal.
int board_push(Board *board, move_t move, Boardstack *stack);

// Same as board_push(), but uses a nullmove instead.
int board_push_nullmove(Board *board, Boardstack *stack);

// Restores the previous position and returns the last move from the stack.
move_t board_pop(Board *board);

// Gets the last move from the stack, or NO_MOVE if we are at the root
// position.
__CU_INLINE move_t board_peek_move(const Board *board) {
    return board->stack->lastMove;
}

// Gets all the moves played from the stack, starting from the root position.
// Returns an allocated pointer to the list of moves, and NULL if an error
// occured or if we are at the root position.
move_t *board_peek_all_moves(Board *board);

// Returns a constant string the FEN representation of the position. The string
// will be invalidated by the next board_to_fen() call, so make sure to copy it
// before successive calls if you need it for a longer time.
const char *board_to_fen(const Board *board);

// Checks if the given pseudo-legal move is a capture.
__CU_INLINE bool board_is_capture(const Board *board, move_t move) {
    return move_type(move) == EN_PASSANT
        || (move_type(move) != CASTLING && !board_is_empty(board, move_to(move)));
}

// Checks if the given pseudo-legal move is neither a capture nor a promotion.
__CU_INLINE bool board_is_quiet(const Board *board, move_t move) {
    return move_type(move) == NORMAL_MOVE
        ? board_is_empty(board, move_to(move))
        : move_type(move) == CASTLING;
}

// Checks if the given pseudo-legal move is a capture or a Pawn move.
__CU_INLINE bool board_is_zeroing(const Board *board, move_t move) {
    return piece_type(board_piece_at(board, move_from(move))) == PAWN
        || (move_type(move) != CASTLING && !board_is_empty(board, move_to(move)));
}

// Checks if the given pseudo-legal move is irreversible.
bool board_is_irreversible(const Board *board, move_t move);

// Checks if the given side attacks the given square.
// Pinned pieces still count as attackers.
bool board_is_attacked_by(const Board *board, square_t sq, color_t c);

// Gets the bitboard of attackers of the given color for the given square.
// Pinned pieces still count as attackers.
bitboard_t board_attackers(const Board *board, square_t sq, color_t c);

// Checks if the given square is pinned to the King of the given color.
// There must be a piece of the same color as the King on the square for the
// test to be valid.
__CU_INLINE bool board_is_pinned(const Board *board, square_t sq, color_t c) {
    return !!(board->stack->checkBlockers[c] & square_bb(sq));
}

// Checks if the given castling is blocked by pieces obstructing the path.
__CU_INLINE bool board_castling_blocked(const Board *board, castling_t castling) {
    return !!(board_occupancy_bb(board) & board->castlingPaths[castling]);
}

__CU_END_DECLS

#endif // __CU_CORE_H__
