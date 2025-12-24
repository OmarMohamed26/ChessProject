#include "utils.h"
#include "draw.h"
#include "load.h"
#include "main.h"
#include "move.h"
#include "settings.h"
#include "stack.h"
#include <stdbool.h>
#include <string.h>

void RestartGame(void)
{
    // 1. Reset Meta-Game Flags (Win/Loss/Draw conditions)
    state.isCheckmate = false;
    state.isStalemate = false;
    state.isRepeated3times = false;
    state.isInsufficientMaterial = false;

    state.isPromoting = false;
    state.promotionRow = -1;
    state.promotionCol = -1;
    state.promotionType = PIECE_NONE;

    state.whitePlayer.Checked = false;
    state.whitePlayer.Checkmated = false;
    state.whitePlayer.SimChecked = false;

    state.blackPlayer.Checked = false;
    state.blackPlayer.Checkmated = false;
    state.blackPlayer.SimChecked = false;

    state.turn = TEAM_BLACK;

    // 2. Clear History Stacks
    ClearStack(state.undoStack);
    ClearStack(state.redoStack);
    // clear DHA will be done in ReadFEN

    // 3. Reset Dead Pieces (use globals as in project)
    deadWhiteCounter = 0;
    deadBlackCounter = 0;
    InitializeDeadPieces(); // Clears the visual arrays

    // 4. Reset Visuals
    ResetJustMoved();
    UpdateLastMoveHighlight(-1, -1);
    ResetSelectedPiece();

    // 5. Reload Board & Game Rules via FEN
    // ReadFEN will initialize pieces, turn, castling, en passant, clocks and DHA as needed.
    UnloadBoard();
    const char startFen[] = STARTING_FEN;
    ReadFEN(startFen, (int)strlen(startFen), false);

    // 6. Initial Validation: compute legal moves for the starting position
    ResetsAndValidations();
}
