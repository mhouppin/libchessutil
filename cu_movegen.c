#include "cu_movegen.h"

__CU_INLINE move_t *__mlist_gen_promotions(move_t *iter, square_t to, direction_t dir) {
    *(iter++) = create_promotion(to - dir, to, KNIGHT);
    *(iter++) = create_promotion(to - dir, to, BISHOP);
    *(iter++) = create_promotion(to - dir, to, ROOK);
    *(iter++) = create_promotion(to - dir, to, QUEEN);

    return iter;
}

__CU_INLINE move_t *__mlist_gen_piece_moves(move_t *iter, const Board *board, color_t us, piecetype_t pt, bitboard_t target) {
    bitboard_t bb = board_piece_bb(board, us, pt);

    while (bb) {
        square_t from = bb_pop_first_square(&bb);
        bitboard_t toBB = attacks_bb(pt, from, board_occupancy_bb(board)) & target;

        while (toBB)
            *(iter++) = create_move(from, bb_pop_first_square(&toBB), NORMAL_MOVE);
    }

    return iter;
}

move_t *__mlist_gen_pawn_moves(move_t *iter, const Board *board, color_t us) {
    direction_t pawnPush = pawn_direction(us);

    bitboard_t rank7PawnsBB    = board_piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t notRank7PawnsBB = board_piece_bb(board, us, PAWN) & ~rank7PawnsBB;
    bitboard_t emptyBB         = ~board_occupancy_bb(board);
    bitboard_t theirPiecesBB   = board_color_bb(board, flip_color(us));
    bitboard_t pushBB          = bb_relative_shift_north(notRank7PawnsBB, us) & emptyBB;
    bitboard_t push2BB         = bb_relative_shift_north(pushBB & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & emptyBB;

    while (pushBB) {
        square_t to = bb_pop_first_square(&pushBB);
        *(iter++) = create_move(to - pawnPush, to, NORMAL_MOVE);
    }

    while (push2BB) {
        square_t to = bb_pop_first_square(&push2BB);
        *(iter++) = create_move(to - pawnPush * 2, to, NORMAL_MOVE);
    }

    if (rank7PawnsBB) {
        bitboard_t promoteBB = bb_relative_shift_north(rank7PawnsBB, us);

        for (bitboard_t bb = promoteBB & emptyBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush);

        for (bitboard_t bb = bb_shift_west(promoteBB) & theirPiecesBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush + WEST);

        for (bitboard_t bb = bb_shift_east(promoteBB) & theirPiecesBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush + EAST);
    }

    bitboard_t captureBB = bb_relative_shift_north(notRank7PawnsBB, us);

    for (bitboard_t bb = bb_shift_west(captureBB) & theirPiecesBB; bb; ) {
        square_t to = bb_pop_first_square(&bb);
        *(iter++) = create_move(to - pawnPush - WEST, to, NORMAL_MOVE);
    }

    for (bitboard_t bb = bb_shift_east(captureBB) & theirPiecesBB; bb; ) {
        square_t to = bb_pop_first_square(&bb);
        *(iter++) = create_move(to - pawnPush - EAST, to, NORMAL_MOVE);
    }

    if (board->stack->enPassantSq != SQ_NONE) {
        bitboard_t captureEpBB = notRank7PawnsBB & pawn_moves_bb(board->stack->enPassantSq, flip_color(us));

        while (captureEpBB)
            *(iter++) = create_move(bb_pop_first_square(&captureEpBB), board->stack->enPassantSq, EN_PASSANT);
    }

    return iter;
}

move_t *__mlist_gen_pawn_evasions(move_t *iter, const Board *board, bitboard_t blockSquares, color_t us) {
    direction_t pawnPush = pawn_direction(us);

    bitboard_t rank7PawnsBB    = board_piece_bb(board, us, PAWN) & (us == WHITE ? RANK_7_BB : RANK_2_BB);
    bitboard_t notRank7PawnsBB = board_piece_bb(board, us, PAWN) & ~rank7PawnsBB;
    bitboard_t emptyBB         = ~board_occupancy_bb(board);
    bitboard_t theirPiecesBB   = board_color_bb(board, flip_color(us)) & blockSquares;
    bitboard_t pushBB          = bb_relative_shift_north(notRank7PawnsBB, us) & emptyBB;
    bitboard_t push2BB         = bb_relative_shift_north(pushBB & (us == WHITE ? RANK_3_BB : RANK_6_BB), us) & emptyBB;

    pushBB  &= blockSquares;
    push2BB &= blockSquares;

    while (pushBB) {
        square_t to = bb_pop_first_square(&pushBB);
        *(iter++) = create_move(to - pawnPush, to, NORMAL_MOVE);
    }

    while (push2BB) {
        square_t to = bb_pop_first_square(&push2BB);
        *(iter++) = create_move(to - pawnPush * 2, to, NORMAL_MOVE);
    }

    if (rank7PawnsBB) {
        emptyBB &= blockSquares;

        bitboard_t promoteBB = bb_relative_shift_north(rank7PawnsBB, us);

        for (bitboard_t bb = promoteBB & emptyBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush);

        for (bitboard_t bb = bb_shift_west(promoteBB) & theirPiecesBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush + WEST);

        for (bitboard_t bb = bb_shift_east(promoteBB) & theirPiecesBB; bb; )
            iter = __mlist_gen_promotions(iter, bb_pop_first_square(&bb), pawnPush + EAST);
    }

    bitboard_t captureBB = bb_relative_shift_north(notRank7PawnsBB, us);

    for (bitboard_t bb = bb_shift_west(captureBB) & theirPiecesBB; bb; ) {
        square_t to = bb_pop_first_square(&bb);
        *(iter++) = create_move(to - pawnPush - WEST, to, NORMAL_MOVE);
    }

    for (bitboard_t bb = bb_shift_east(captureBB) & theirPiecesBB; bb; ) {
        square_t to = bb_pop_first_square(&bb);
        *(iter++) = create_move(to - pawnPush - EAST, to, NORMAL_MOVE);
    }

    if (board->stack->enPassantSq != SQ_NONE) {
        if (!(blockSquares & square_bb(board->stack->enPassantSq - pawnPush)))
            return iter;

        bitboard_t captureEpBB = notRank7PawnsBB & pawn_moves_bb(board->stack->enPassantSq, flip_color(us));

        while (captureEpBB)
            *(iter++) = create_move(bb_pop_first_square(&captureEpBB), board->stack->enPassantSq, EN_PASSANT);
    }

    return iter;
}

move_t *__mlist_gen_moves(move_t *iter, const Board *board) {
    color_t us = board_turn(board);
    bitboard_t target = ~board_color_bb(board, us);
    square_t kingSq = board_king_square(board, us);

    iter = __mlist_gen_pawn_moves(iter, board, us);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        iter = __mlist_gen_piece_moves(iter, board, us, pt, target);

    for (bitboard_t bb = king_moves_bb(kingSq) & target; bb; )
        *(iter++) = create_move(kingSq, bb_pop_first_square(&bb), NORMAL_MOVE);

    castling_t kingside  = castling_color_mask(us) & KINGSIDE_CASTLING;
    castling_t queenside = castling_color_mask(us) & QUEENSIDE_CASTLING;

    if (!!(board->stack->castlingRights & kingside) && !board_castling_blocked(board, kingside))
        *(iter++) = create_move(kingSq, board->castlingRookSquare[kingside], CASTLING);

    if (!!(board->stack->castlingRights & queenside) && !board_castling_blocked(board, queenside))
        *(iter++) = create_move(kingSq, board->castlingRookSquare[queenside], CASTLING);

    return iter;
}

move_t *__mlist_gen_evasions(move_t *iter, const Board *board) {
    color_t us = board_turn(board);
    square_t kingSq = board_king_square(board, us);
    bitboard_t sliderAttacks = 0;
    bitboard_t sliders = board->stack->checkers & ~board_piecetypes_bb(board, PAWN, KNIGHT);

    while (sliders) {
        square_t checkSq = bb_pop_first_square(&sliders);
        sliderAttacks |= __cu_line_bb[checkSq][kingSq] ^ square_bb(checkSq); 
    }

    for (bitboard_t bb = king_moves_bb(kingSq) & ~board_color_bb(board, us) & ~sliderAttacks; bb; )
        *(iter++) = create_move(kingSq, bb_pop_first_square(&bb), NORMAL_MOVE);

    // If double check, only a King move can be played
    if (more_than_one_bit(board->stack->checkers))
        return iter;

    square_t checkSq = bb_first_square(board->stack->checkers);
    bitboard_t blockSquares = between_squares_bb(checkSq, kingSq) | square_bb(checkSq);

    iter = __mlist_gen_pawn_evasions(iter, board, blockSquares, us);

    for (piecetype_t pt = KNIGHT; pt <= QUEEN; ++pt)
        iter = __mlist_gen_piece_moves(iter, board, us, pt, blockSquares);

    return iter;
}

void mlist_generate_pseudo_legal(Movelist *mlist, const Board *board) {
    mlist->end = board->stack->checkers
        ? __mlist_gen_evasions(mlist->moves, board)
        : __mlist_gen_moves(mlist->moves, board);
}

void mlist_generate_legal(Movelist *mlist, const Board *board) {
    color_t us = board_turn(board);
    bitboard_t pinned = board->stack->checkBlockers[us] & board_color_bb(board, us);
    square_t kingSq = board_king_square(board, us);
    move_t *iter = mlist->moves;

    mlist->end = board->stack->checkers
        ? __mlist_gen_evasions(mlist->moves, board)
        : __mlist_gen_moves(mlist->moves, board);

    while (iter < mlist->end) {
        if ((pinned || move_from(*iter) == kingSq || move_type(*iter) == EN_PASSANT)
            && !board_move_is_legal(board, *iter))
            *iter = *(--mlist->end);
        else
            ++iter;
    }
}
