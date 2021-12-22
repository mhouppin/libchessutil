#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cu_core.h"
#include "cu_movegen.h"

const char *const STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const char PIECE_INDEXES[PIECE_NB] = " PNBRQK  pnbrqk";

__CU_INLINE Boardstack *__boardstack_dup(Boardstack *stack) {
    if (!stack)
        return NULL;

    Boardstack *newStack = malloc(sizeof(Boardstack));
    if (newStack == NULL)
        return NULL;

    *newStack = *stack;
    newStack->prev = __boardstack_dup(stack->prev);
    if (newStack->prev == NULL && stack->prev != NULL) {
        free(newStack);
        return NULL;
    }

    return (newStack);
}

__CU_INLINE void __boardstack_free(Boardstack *stack) {
    while (stack) {
        Boardstack *prev = stack->prev;
        free(stack);
        stack = prev;
    }
}

__CU_INLINE void __board_put_piece(Board *board, piece_t pc, square_t sq) {
    board->table[sq] = pc;
    board->piecetypeBBs[ALL_PIECES]     |= square_bb(sq);
    board->piecetypeBBs[piece_type(pc)] |= square_bb(sq);
    board->colorBBs[piece_color(pc)]    |= square_bb(sq);
    board->pieceCounts[pc]++;
    board->pieceCounts[create_piece(piece_color(pc), ALL_PIECES)]++;
}

__CU_INLINE void __board_move_piece(Board *board, square_t from, square_t to) {
    piece_t pc = board_piece_at(board, from);
    bitboard_t moveBB = square_bb(from) | square_bb(to);

    board->piecetypeBBs[ALL_PIECES]     ^= moveBB;
    board->piecetypeBBs[piece_type(pc)] ^= moveBB;
    board->colorBBs[piece_color(pc)]    ^= moveBB;
    board->table[from] = NO_PIECE;
    board->table[to]   = pc;
}

__CU_INLINE void __board_remove_piece(Board *board, square_t sq) {
    piece_t pc = board_piece_at(board, sq);

    board->piecetypeBBs[ALL_PIECES]     ^= square_bb(sq);
    board->piecetypeBBs[piece_type(pc)] ^= square_bb(sq);
    board->colorBBs[piece_color(pc)]    ^= square_bb(sq);
    board->pieceCounts[pc]--;
    board->pieceCounts[create_piece(piece_color(pc), ALL_PIECES)]--;
}

int __board_set_castling(Board *board, color_t c, square_t rookSq) {
    square_t kingSq = board_king_square(board, c);
    castling_t castling = castling_color_mask(c)
        & (kingSq < rookSq ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING);

    if (relative_square_rank(kingSq, c) != RANK_1) {
        board_set_error(board, "Castling rights set with King not on back-rank");
        return -1;
    }

    // If the King or the Rook are not on their usual squares, we're playing
    // Chess960.
    if (square_file(kingSq) != FILE_E
        || (square_file(rookSq) != FILE_A && square_file(rookSq) != FILE_H))
        board->chess960 = true;

    board->stack->castlingRights |= castling;
    board->castlingMasks[kingSq] |= castling;
    board->castlingMasks[rookSq] |= castling;
    board->castlingRookSquare[castling] = rookSq;

    square_t kingAfter = relative_square(castling & KINGSIDE_CASTLING ? SQ_G1 : SQ_C1, c);
    square_t rookAfter = relative_square(castling & KINGSIDE_CASTLING ? SQ_F1 : SQ_D1, c);

    board->castlingPaths[castling] = (
            between_squares_bb(rookSq, rookAfter) | square_bb(rookAfter)
          | between_squares_bb(kingSq, kingAfter) | square_bb(kingAfter)
        ) & ~(square_bb(kingSq) | square_bb(rookSq));

    return 0;
}

bitboard_t __board_slider_blockers(const Board *board, bitboard_t sliders, square_t sq, bitboard_t *pinners) {
    bitboard_t blockers = *pinners = 0;
    bitboard_t snipers = sliders & (
        (__cu_pseudo_moves_bb[BISHOP][sq] & board_piecetypes_bb(board, BISHOP, QUEEN))
      | (__cu_pseudo_moves_bb[ROOK  ][sq] & board_piecetypes_bb(board, ROOK,   QUEEN)));
    bitboard_t occupancy = board_occupancy_bb(board) ^ snipers;

    while (snipers) {
        square_t sniperSq = bb_pop_first_square(&snipers);
        bitboard_t between = between_squares_bb(sniperSq, sq) & occupancy;

        // No pins if there are two or more pieces between the slider and the target square.
        if (between && !more_than_one_bit(between)) {
            blockers |= between;
            if (between & board_color_bb(board, piece_color(board_piece_at(board, sq))))
                *pinners |= square_bb(sniperSq);
        }
    }

    return blockers;
}

void __board_set_check(Board *board, Boardstack *stack) {
    color_t them = flip_color(board_turn(board));

    stack->checkBlockers[WHITE] = __board_slider_blockers(board,
        board_color_bb(board, BLACK),
        board_king_square(board, WHITE),
        &stack->checkPinners[BLACK]);
    stack->checkBlockers[BLACK] = __board_slider_blockers(board,
        board_color_bb(board, WHITE),
        board_king_square(board, BLACK),
        &stack->checkPinners[WHITE]);

    square_t kingSq = board_king_square(board, them);

    stack->checkSquares[PAWN]   = pawn_moves_bb(kingSq, them);
    stack->checkSquares[KNIGHT] = knight_moves_bb(kingSq);
    stack->checkSquares[BISHOP] = bishop_moves_bb(kingSq, board_occupancy_bb(board));
    stack->checkSquares[ROOK]   = rook_moves_bb(kingSq, board_occupancy_bb(board));
    stack->checkSquares[QUEEN]  = stack->checkSquares[BISHOP] | stack->checkSquares[ROOK];
    stack->checkSquares[KING]   = 0;
}

int __board_set_stack(Board *board, Boardstack *stack) {
    color_t us = board_turn(board), them = flip_color(board_turn(board));
    stack->key = stack->materialKey = 0;
    stack->checkers = board_attackers(board, board_king_square(board, us), them);

    // If we're attacking the opponent's King and it's our turn to move, the
    // FEN is invalid.
    if (board_attackers(board, board_king_square(board, them), us)) {
        board_set_error(board, "Side to move can already capture the enemy King");
        return -1;
    }

    __board_set_check(board, stack);

    // Initialize board Zobrist key.
    for (bitboard_t bb = board_occupancy_bb(board); bb; ) {
        square_t sq = bb_pop_first_square(&bb);
        piece_t pc = board_piece_at(board, sq);

        stack->key ^= __cu_zobrist_psq[pc][sq];
    }

    if (stack->enPassantSq != SQ_NONE)
        stack->key ^= __cu_zobrist_ep[square_file(stack->enPassantSq)];

    if (board_turn(board) == BLACK)
        stack->key ^= __cu_zobrist_turn;

    stack->key ^= __cu_zobrist_castling[stack->castlingRights];

    // Initialize material Zobrist key.

    for (color_t c = WHITE; c <= BLACK; ++c)
        for (piecetype_t pt = PAWN; pt <= KING; ++pt)
        {
            piece_t pc = create_piece(c, pt);

            for (int i = 0; i < board_count_piece(board, pc); ++i)
                stack->materialKey ^= __cu_zobrist_psq[pc][i];
        }

    return 0;
}

int board_from_fen(Board *board, Boardstack *stack, const char *fen) {
    const char *whitespaces = " \t\r\n";

    memset(board, 0, sizeof(Board));

    if (stack == NULL) {
        board->internalStackAllocator = true;
        stack = calloc(1, sizeof(Boardstack));
        if (stack == NULL) {
            board_set_error(board, "Out of memory");
            return -2;
        }
    }
    else
        memset(stack, 0, sizeof(Boardstack));

    board->stack = stack;

    // Skip any trailing whitespaces. Note that we don't skip '\f'
    // and '\v', because their presence might suggest something went
    // wrong during the FEN loading.
    fen += strspn(fen, whitespaces);

    char *ptr;
    square_t sq = SQ_A8;
    size_t nextSection = strcspn(fen, whitespaces);

    // Parse the piece section of the FEN.
    for (size_t i = 0; i < nextSection; ++i) {
        if (fen[i] >= '1' && fen[i] <= '8')
            sq += fen[i] - '0';

        else if (fen[i] == '/')
            sq += 2 * SOUTH;

        else if (ptr = strchr(PIECE_INDEXES, fen[i]), ptr != NULL)
            __board_put_piece(board, (piece_t)(ptr - PIECE_INDEXES), sq++);

        else {
            board_set_error(board, "Invalid character in piece section");
            return -1;
        }
    }

    // Check if there's a correct number of Kings on the board.
    if (board_count_piece(board, WHITE_KING) != 1 || board_count_piece(board, BLACK_KING) != 1) {
        board_set_error(board, "Invalid number of Kings on the board");
        return -1;
    }

    // Check if the Kings aren't next to each other.
    if (square_distance(board_king_square(board, WHITE), board_king_square(board, BLACK)) == 1) {
        board_set_error(board, "Kings cannot touch each other");
        return -1;
    }

    // From here, if the remaining sections are missing, we just assume default
    // values for them.
    fen += nextSection;
    fen += strspn(fen, whitespaces);
    nextSection = strcspn(fen, whitespaces);

    // Parse the side to move section of the FEN.
    if (nextSection > 1) {
        board_set_error(board, "Too many characters for side to move section");
        return -1;
    }

    if (*fen == 'b')
        board->sideToMove = BLACK;

    else if (*fen == 'w' || *fen == '\0')
        board->sideToMove = WHITE;

    else {
        board_set_error(board, "Invalid character in side to move section");
        return -1;
    }

    fen += nextSection;
    fen += strspn(fen, whitespaces);
    nextSection = strcspn(fen, whitespaces);

    // Parse the castling section of the FEN.
    for (size_t i = 0; i < nextSection; ++i) {
        if (fen[i] == '-') {
            // If '-' is specified, it should be the only character of the section.
            if (nextSection > 1) {
                board_set_error(board, "'-' specified in castling section with extra characters");
                return -1;
            }
            break ;
        }

        square_t rookSq;
        color_t c = islower(fen[i]) ? BLACK : WHITE;
        piece_t rook = create_piece(c, ROOK);
        char castlingChar = toupper(fen[i]);

        if (castlingChar == 'K')
            for (rookSq = relative_square(SQ_H1, c); board_piece_at(board, rookSq) != rook; --rookSq);

        else if (castlingChar == 'Q')
            for (rookSq = relative_square(SQ_A1, c); board_piece_at(board, rookSq) != rook; ++rookSq);

        else if (castlingChar >= 'A' && castlingChar <= 'H')
            rookSq = create_square((file_t)(castlingChar - 'A'), relative_rank(RANK_1, c));

        else {
            board_set_error(board, "Invalid character in castling section");
            return -1;
        }

        if (__board_set_castling(board, c, rookSq))
            return -1;
    }

    fen += nextSection;
    fen += strspn(fen, whitespaces);
    nextSection = strcspn(fen, whitespaces);

    board->stack->enPassantSq = SQ_NONE;

    // Parse the e.p. section of the FEN.
    if (nextSection == 1 && *fen != '-') {
        board_set_error(board, "Invalid character in e.p. section");
        return -1;
    }
    else if (nextSection > 2) {
        board_set_error(board, "Too many characters in e.p. section");
        return -1;
    }
    else if (nextSection == 2) {
        char fileChar = tolower(fen[0]);
        char rankChar = fen[1];

        if (fileChar < 'a' || fileChar > 'h' || rankChar != (board->sideToMove == WHITE ? '6' : '3')) {
            board_set_error(board, "Invalid e.p. square");
            return -1;
        }

        board->stack->enPassantSq = create_square((file_t)(fileChar - 'a'), (rank_t)(rankChar - '1'));
        board->stack->polyglotEP = board->stack->enPassantSq;
        color_t us = board_turn(board), them = flip_color(board_turn(board));
        square_t epSq = board->stack->enPassantSq;

        if (!(square_bb(epSq) & pawn_attacks_bb(board_piece_bb(board, us, PAWN), us)))
            board->stack->enPassantSq = SQ_NONE;

        else if (!(board_piece_bb(board, them, PAWN) & square_bb(epSq + pawn_direction(them)))) {
            board_set_error(board, "e.p. square set even though no pawn is present on the front square");
            return -1;
        }
    }

    fen += nextSection;
    fen += strspn(fen, whitespaces);
    nextSection = strcspn(fen, whitespaces);

    // Parse the rule50 section of the FEN.
    board->stack->rule50 = strtol(fen, &ptr, 10);

    if (fen + nextSection != ptr || board->stack->rule50 < 0) {
        board_set_error(board, "Invalid rule50 data");
        return -1;
    }

    fen += nextSection;
    fen += strspn(fen, whitespaces);
    nextSection = strcspn(fen, whitespaces);

    // Parse the move number section of the FEN.
    board->gamePly = strtol(fen, &ptr, 10);

    if (fen + nextSection != ptr || board->gamePly < 0) {
        board_set_error(board, "Invalid move number data");
        return -1;
    }

    board->gamePly = __cu_max(0, 2 * board->gamePly - 2);
    board->gamePly += board->sideToMove == BLACK;

    return __board_set_stack(board, board->stack);
}

void board_destroy(Board *board) {
    board_set_error(board, NULL);

    if (board->internalStackAllocator) {
        __boardstack_free(board->stack);
        board->stack = NULL;
    }
}

void board_reset(Board *board) {
    while (board->stack->prev)
        board_pop(board);
}

void board_set_error(Board *board, const char *str) {
    if (str == NULL) {
        board->err[0] = '\0';
    }
    else
        strcpy(board->err, str);
}

int board_copy_root(Board *dst, const Board *src, Boardstack *stack) {
    *dst = *src;
    dst->internalStackAllocator = false;
    board_reset(dst);

    if (!stack) {
        dst->internalStackAllocator = true;
        dst->stack = __boardstack_dup(src->stack);

        if (dst->stack == NULL) {
            board_set_error(dst, "Out of memory");
            return -2;
        }
    }
    else {
        *stack = *dst->stack;
        dst->stack = stack;
    }

    return 0;
}

int board_copy(Board *dst, const Board *src) {
    *dst = *src;
    dst->internalStackAllocator = true;
    dst->stack = __boardstack_dup(src->stack);

    if (dst->stack == NULL) {
        board_set_error(dst, "Out of memory");
        return -2;
    }

    return 0;
}

bool board_move_is_legal(const Board *board, move_t move) {
    color_t us = board_turn(board), them = flip_color(board_turn(board));
    square_t from = move_from(move), to = move_to(move);

    if (move_type(move) == EN_PASSANT) {
        // Test for any discovered check with the en-passant capture.
        square_t kingSq = board_king_square(board, us);
        square_t captureSq = to - pawn_direction(us);
        bitboard_t occupancy = (board_occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSq)) | square_bb(to);

        return !(bishop_moves_bb(kingSq, occupancy) & board_pieces_bb(board, them, BISHOP, QUEEN))
            && !(rook_moves_bb(kingSq, occupancy) & board_pieces_bb(board, them, ROOK, QUEEN));
    }

    if (move_type(move) == CASTLING) {
        // Test for any opponent piece attack along the King path.
        to = relative_square(to > from ? SQ_G1 : SQ_C1, us);

        direction_t side = to > from ? WEST : EAST;

        for (square_t sq = to; sq != from; sq += side)
            if (board_attackers(board, sq, them))
                return false;

        return !board->chess960
            || !(rook_moves_bb(to, board_occupancy_bb(board) ^ square_bb(move_to(move)))
                & board_pieces_bb(board, them, ROOK, QUEEN));
    }

    // Test for any opponent piece attack on the arrival King square.
    if (piece_type(board_piece_at(board, from)) == KING)
        return !board_attackers(board, to, them);

    // If the moving piece is pinned, test if the move generates a discovered
    // check.
    return !(board->stack->checkBlockers[us] & square_bb(from)) || squares_aligned(from, to, board_king_square(board, us));
}

bool board_move_gives_check(const Board *board, move_t move) {
    square_t from = move_from(move), to = move_to(move);
    square_t captureSq, theirKing;
    square_t kingFrom, rookFrom, kingTo, rookTo;
    bitboard_t occupancy;
    color_t us = board_turn(board), them = flip_color(board_turn(board));

    // Test if the move is a direct check.
    if (board->stack->checkSquares[piece_type(board_piece_at(board, from))] & square_bb(to))
        return true;

    // Test if the move is a discovered check.
    theirKing = board_king_square(board, them);
    if (!!(board->stack->checkBlockers[them] & square_bb(from)) && !squares_aligned(from, to, theirKing))
        return true;

    switch (move_type(move)) {
        case PROMOTION:
            return !!(attacks_bb(promotion_type(move), to, board_occupancy_bb(board) ^ square_bb(from))
                    & square_bb(theirKing));

        case EN_PASSANT:
            captureSq = create_square(square_file(to), square_rank(from));
            occupancy = (board_occupancy_bb(board) ^ square_bb(from) ^ square_bb(captureSq)) | square_bb(to);

            return !!((bishop_moves_bb(theirKing, occupancy) & board_pieces_bb(board, us, BISHOP, QUEEN))
                    | (rook_moves_bb  (theirKing, occupancy) & board_pieces_bb(board, us, ROOK,   QUEEN)));

        case CASTLING:
            kingFrom = from;
            rookFrom = to;
            kingTo = relative_square(rookFrom > kingFrom ? SQ_G1 : SQ_C1, us);
            rookTo = relative_square(rookFrom > kingFrom ? SQ_F1 : SQ_D1, us);

        return !!(__cu_pseudo_moves_bb[ROOK][rookTo] & square_bb(theirKing))
            && !!(rook_moves_bb(rookTo,
                    (board_occupancy_bb(board) ^ square_bb(kingFrom) ^ square_bb(rookFrom))
                    | square_bb(kingTo) | square_bb(rookTo))
                & square_bb(theirKing));

        // Only for normal moves
        default:
            return false;
    }
}

bool board_is_checkmate(const Board *board) {
    if (!board->stack->checkers)
        return false;

    Movelist mlist;

    mlist_generate_legal(&mlist, board);
    return !mlist_size(&mlist);
}

bool board_is_stalemate(const Board *board) {
    if (board->stack->checkers)
        return false;

    Movelist mlist;

    mlist_generate_legal(&mlist, board);
    return !mlist_size(&mlist);
}

bool board_is_material_draw(const Board *board) {
    if (board_piecetype_bb(board, PAWN) || board_piecetypes_bb(board, ROOK, QUEEN))
        return false;

    int pieceCount = board_count_all(board);

    if (pieceCount <= 3)
        return true;

    if (board_piecetype_bb(board, KNIGHT))
        return false;

    bitboard_t bishopsBB = board_piecetype_bb(board, BISHOP);

    return !(bishopsBB & LIGHT_SQUARES_BB) || !(bishopsBB & DARK_SQUARES_BB);
}

bool board_is_rule75_draw(const Board *board) {
    if (board_rule50(board) < 150)
        return false;

    Movelist mlist;

    mlist_generate_legal(&mlist, board);
    return !!mlist_size(&mlist);
}

bool board_is_rule50_draw(const Board *board) {
    if (board_rule50(board) < 100)
        return false;

    Movelist mlist;

    mlist_generate_legal(&mlist, board);
    return !!mlist_size(&mlist);
}

outcome_t board_outcome(const Board *board, bool claimDraw) {
    Movelist mlist;

    mlist_generate_legal(&mlist, board);

    // Test if the side to move is checkmated/stalemated.
    if (!mlist_size(&mlist)) {
        if (!board->stack->checkers)
            return DRAWN_GAME;

        return board->sideToMove == WHITE ? BLACK_WINS : WHITE_WINS;
    }

    if (board_rule50(board) >= 150 || board->stack->repetition >= 5)
        return DRAWN_GAME;

    if (claimDraw && (board_rule50(board) >= 100 || board->stack->repetition >= 3))
        return DRAWN_GAME;

    return NO_OUTCOME;
}

int board_push(Board *board, move_t move, Boardstack *stack) {
    if (board->internalStackAllocator) {
        stack = malloc(sizeof(Boardstack));

        if (stack == NULL) {
            board_set_error(board, "Out of memory");
            return -2;
        }
    }

    bool givesCheck = board_move_gives_check(board, move);
    hashkey_t key = board->stack->key ^ __cu_zobrist_turn;

    stack->castlingRights = board->stack->castlingRights;
    stack->rule50         = board->stack->rule50 + 1;
    stack->lastNullmove   = board->stack->lastNullmove + 1;
    stack->enPassantSq    = board->stack->enPassantSq;
    stack->materialKey    = board->stack->materialKey;

    stack->lastMove = move;
    stack->prev = board->stack;
    stack->polyglotEP = SQ_NONE;
    board->stack = stack;
    ++board->gamePly;

    color_t us = board_turn(board), them = flip_color(board_turn(board));
    square_t from = move_from(move), to = move_to(move);
    piece_t pc = board_piece_at(board, from);
    piece_t captured = move_type(move) == EN_PASSANT ? create_piece(them, PAWN) : board_piece_at(board, to);

    if (move_type(move) == CASTLING) {
        bool kingside = to > from;

        square_t rookFrom = to;
        square_t rookTo   = relative_square(kingside ? SQ_F1 : SQ_D1, us);
        to                = relative_square(kingside ? SQ_G1 : SQ_C1, us);

        __board_remove_piece(board, from);
        __board_remove_piece(board, rookFrom);
        board->table[from] = board->table[rookFrom] = NO_PIECE;
        __board_put_piece(board, create_piece(us, KING), to);
        __board_put_piece(board, create_piece(us, ROOK), rookTo);

        key ^= __cu_zobrist_psq[captured][rookFrom] ^ __cu_zobrist_psq[captured][rookTo];
        captured = NO_PIECE;
    }

    if (captured) {
        square_t captureSq = to;

        if (move_type(move) == EN_PASSANT)
            captureSq -= pawn_direction(us);

        __board_remove_piece(board, captureSq);

        if (move_type(move) == EN_PASSANT)
            board->table[captureSq] = NO_PIECE;

        key ^= __cu_zobrist_psq[captured][captureSq];
        stack->materialKey ^= __cu_zobrist_psq[captured][board_count_piece(board, captured)];
        stack->rule50 = 0;
    }

    key ^= __cu_zobrist_psq[pc][from] ^ __cu_zobrist_psq[pc][to];

    if (stack->enPassantSq != SQ_NONE) {
        key ^= __cu_zobrist_ep[square_file(board->stack->enPassantSq)];
        stack->enPassantSq = SQ_NONE;
    }

    if (stack->castlingRights && (board->castlingMasks[from] | board->castlingMasks[to])) {
        castling_t castling = board->castlingMasks[from] | board->castlingMasks[to];
        key ^= __cu_zobrist_castling[stack->castlingRights & castling];
        stack->castlingRights &= ~castling;
    }

    if (move_type(move) != CASTLING)
        __board_move_piece(board, from, to);

    if (piece_type(pc) == PAWN) {
        if ((to ^ from) == 16) {
            stack->polyglotEP = to - pawn_direction(us);
        
            if (pawn_moves_bb(to - pawn_direction(us), us) & board_piece_bb(board, them, PAWN)) {
                stack->enPassantSq = to - pawn_direction(us);
                key ^= __cu_zobrist_ep[square_file(stack->enPassantSq)];
            }
        }
        else if (move_type(move) == PROMOTION) {
            piece_t newPc = create_piece(us, promotion_type(move));

            __board_remove_piece(board, to);
            __board_put_piece(board, newPc, to);

            key ^= __cu_zobrist_psq[pc][to] ^ __cu_zobrist_psq[newPc][to];
            stack->materialKey ^= __cu_zobrist_psq[newPc][board_count_piece(board, newPc) - 1];
            stack->materialKey ^= __cu_zobrist_psq[pc][board_count_piece(board, pc)];
        }

        stack->rule50 = 0;
    }

    stack->capturedPiece = captured;
    stack->key = key;
    stack->checkers = givesCheck ? board_attackers(board, board_king_square(board, them), us) : 0;
    board->sideToMove = flip_color(board->sideToMove);

    __board_set_check(board, stack);

    stack->repetition = 0;

    int repetitionPlies = __cu_min(board->stack->rule50, board->stack->lastNullmove);
    Boardstack *stackIt = stack->prev->prev;

    for (int i = 4; i <= repetitionPlies; i += 2) {
        stackIt = stackIt->prev->prev;

        if (stackIt->key == stack->key) {
            board->stack->repetition = stackIt->repetition + 1;
            break ;
        }
    }

    return 0;
}

int board_push_nullmove(Board *board, Boardstack *stack) {
    if (board->internalStackAllocator) {
        stack = malloc(sizeof(Boardstack));

        if (stack == NULL) {
            board_set_error(board, "Out of memory");
            return -2;
        }
    }

    *stack = *board->stack;
    stack->lastMove = NULL_MOVE;
    stack->prev = board->stack;
    board->stack = stack;

    if (stack->enPassantSq != SQ_NONE) {
        stack->key ^= __cu_zobrist_ep[square_file(board->stack->enPassantSq)];
        stack->enPassantSq = SQ_NONE;
    }

    stack->key ^= __cu_zobrist_turn;
    ++stack->rule50;
    stack->lastNullmove = 0;
    board->sideToMove = flip_color(board->sideToMove);

    __board_set_check(board, stack);

    stack->repetition = 0;

    return 0;
}

move_t board_pop(Board *board) {
    Boardstack *last = board->stack;

    board->stack = board->stack->prev;
    board->sideToMove = flip_color(board->sideToMove);

    if (last->lastMove == NULL_MOVE) {
        if (board->internalStackAllocator)
            free(last);
        return NULL_MOVE;
    }

    move_t move = last->lastMove;
    color_t us = board_turn(board);
    square_t from = move_from(move), to = move_to(move);

    if (move_type(move) == PROMOTION) {
        __board_remove_piece(board, to);
        __board_put_piece(board, create_piece(us, PAWN), to);
    }

    if (move_type(move) == CASTLING) {
        bool kingside = to > from;

        square_t rookFrom = to;
        square_t rookTo   = relative_square(kingside ? SQ_F1 : SQ_D1, us);
        to                = relative_square(kingside ? SQ_G1 : SQ_C1, us);

        __board_remove_piece(board, to);
        __board_remove_piece(board, rookTo);
        board->table[to] = board->table[rookTo] = NO_PIECE;
        __board_put_piece(board, create_piece(us, KING), from);
        __board_put_piece(board, create_piece(us, ROOK), rookFrom);
    }
    else {
        __board_move_piece(board, to, from);

        if (last->capturedPiece) {
            square_t captureSq = to;

            if (move_type(move) == EN_PASSANT)
                captureSq -= pawn_direction(us);

            __board_put_piece(board, last->capturedPiece, captureSq);
        }
    }

    if (board->internalStackAllocator)
        free(last);

    --board->gamePly;
    return move;
}

move_t *board_peek_all_moves(Board *board) {
    size_t moveCount = 0;

    for (const Boardstack *it = board->stack; it->prev != NULL; it = it->prev)
        ++moveCount;

    if (moveCount == 0)
        return NULL;

    move_t *moves = malloc(sizeof(move_t) * moveCount);

    if (moves == NULL) {
        board_set_error(board, "Out of memory");
        return NULL;
    }

    for (const Boardstack *it = board->stack; it->prev != NULL; it = it->prev)
        moves[--moveCount] = it->lastMove;

    return moves;
}

const char *board_to_fen(const Board *board) {
    static char fenBuffer[128];
    char *ptr = fenBuffer;

    for (rank_t r = RANK_8; r <= RANK_8; --r) {
        for (file_t f = FILE_A; f <= FILE_H; ++f) {
            int emptyCount;

            for (emptyCount = 0; f <= FILE_H && board_is_empty(board, create_square(f, r)); ++f)
                ++emptyCount;

            if (emptyCount)
                *(ptr++) = emptyCount + '0';

            if (f <= FILE_H)
                *(ptr++) = PIECE_INDEXES[board_piece_at(board, create_square(f, r))];
        }

        if (r > RANK_1)
            *(ptr++) = '/';
    }

    *(ptr++) = ' ';
    *(ptr++) = board_turn(board) == WHITE ? 'w' : 'b';
    *(ptr++) = ' ';

    if (board->stack->castlingRights & WHITE_OO)
        *(ptr++) = board->chess960 ? 'A' + square_file(board->castlingRookSquare[WHITE_OO]) : 'K';

    if (board->stack->castlingRights & WHITE_OOO)
        *(ptr++) = board->chess960 ? 'A' + square_file(board->castlingRookSquare[WHITE_OOO]) : 'Q';

    if (board->stack->castlingRights & BLACK_OO)
        *(ptr++) = board->chess960 ? 'a' + square_file(board->castlingRookSquare[BLACK_OO]) : 'k';

    if (board->stack->castlingRights & BLACK_OOO)
        *(ptr++) = board->chess960 ? 'a' + square_file(board->castlingRookSquare[BLACK_OOO]) : 'q';

    if (!board->stack->castlingRights)
        *(ptr++) = '-';

    *(ptr++) = ' ';

    if (board->stack->enPassantSq == SQ_NONE)
        *(ptr++) = '-';

    else {
        *(ptr++) = 'a' + square_file(board->stack->enPassantSq);
        *(ptr++) = '1' + square_rank(board->stack->enPassantSq);
    }

    sprintf(ptr, " %d %d", board_rule50(board), 1 + (board_ply(board) - (board_turn(board) == BLACK)) / 2);

    return fenBuffer;
}

bool board_is_irreversible(const Board *board, move_t move) {

    // All promotions, castling moves and en-passant captures are irreversible.
    if (move_type(move) != NORMAL_MOVE)
        return true;

    // Pawn moves are irreversible.
    if (piece_type(board_piece_at(board, move_from(move))) == PAWN)
        return true;

    // King/Rook moves destroying castling rights are irreversible.
    if (board->stack->castlingRights && (board->castlingMasks[move_from(move)] | board->castlingMasks[move_to(move)]))
        return true;

    // Captures are irreversible.
    if (board_piece_at(board, move_to(move)) != NO_PIECE)
        return true;

    // Moves are always irreversible if we have a legal en-passant move.
    if (board->stack->enPassantSq != SQ_NONE) {
        color_t us = board_turn(board);

        bitboard_t captureEpBB = board_piece_bb(board, us, PAWN)
            & pawn_moves_bb(board->stack->enPassantSq, flip_color(us));

        while (captureEpBB) {
            move_t epMove = create_move(bb_pop_first_square(&captureEpBB), board->stack->enPassantSq, EN_PASSANT);

            if (board_move_is_legal(board, epMove))
                return true;
        }
    }

    return false;
}

bool board_is_attacked_by(const Board *board, square_t sq, color_t c) {
    return !!(pawn_moves_bb(sq, flip_color(c)) & board_piece_bb(board, c, PAWN))
         || !!(knight_moves_bb(sq) & board_piece_bb(board, c, KNIGHT))
         || !!(bishop_moves_bb(sq, board_occupancy_bb(board)) & board_pieces_bb(board, c, BISHOP, QUEEN))
         || !!(rook_moves_bb(sq, board_occupancy_bb(board)) & board_pieces_bb(board, c, ROOK, QUEEN))
         || !!(king_moves_bb(sq) & board_piece_bb(board, c, KING));
}

bitboard_t board_attackers(const Board *board, square_t sq, color_t c) {
    return (pawn_moves_bb(sq, flip_color(c)) & board_piece_bb(board, c, PAWN))
         | (knight_moves_bb(sq) & board_piece_bb(board, c, KNIGHT))
         | (bishop_moves_bb(sq, board_occupancy_bb(board)) & board_pieces_bb(board, c, BISHOP, QUEEN))
         | (rook_moves_bb(sq, board_occupancy_bb(board)) & board_pieces_bb(board, c, ROOK, QUEEN))
         | (king_moves_bb(sq) & board_piece_bb(board, c, KING));
}
