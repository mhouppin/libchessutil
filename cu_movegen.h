#ifndef __CU_MOVEGEN_H__
#define __CU_MOVEGEN_H__

#include <stddef.h>
#include "cu_core.h"

__CU_BEGIN_DECLS

// Structure for storing a list of moves for a position.
typedef struct Movelist_ {
    move_t moves[CU_MAX_MOVES];
    move_t *end;
} Movelist;

// Generate all legal moves from the given position.
void mlist_generate_legal(Movelist *mlist, const Board *board);

// Generate all pseudo-legal moves from the given position.
void mlist_generate_pseudo_legal(Movelist *mlist, const Board *board);

// Returns the number of moves contained in the list.
__CU_INLINE size_t mlist_size(const Movelist *mlist) {
    return (size_t)(mlist->end - (move_t *const)mlist->moves);
}

// Returns a pointer to the first move in the list.
__CU_INLINE move_t *mlist_begin(Movelist *mlist) {
    return mlist->moves;
}

// Returns a const pointer to the first move in the list.
__CU_INLINE const move_t *mlist_cbegin(const Movelist *mlist) {
    return mlist->moves;
}

// Returns a pointer after the last move in the list.
__CU_INLINE move_t *mlist_end(Movelist *mlist) {
    return mlist->end;
}

// Returns a const pointer after the last move in the list.
__CU_INLINE const move_t *mlist_cend(const Movelist *mlist) {
    return mlist->end;
}

// Tests if the list contains the given move.
__CU_INLINE bool mlist_has_move(const Movelist *mlist, move_t move) {
    for (const move_t *iter = mlist_cbegin(mlist); iter < mlist_cend(mlist); ++iter)
        if (*iter == move)
            return true;

    return false;
}

__CU_END_DECLS

#endif
