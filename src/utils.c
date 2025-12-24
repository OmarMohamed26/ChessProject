#include "utils.h"
#include "draw.h"
#include "load.h"
#include "main.h"
#include "move.h"
#include "settings.h"
#include "stack.h"
#include <stdbool.h>
#include <stdlib.h> // For malloc/free
#include <string.h>

void LoadGameFromFEN(const char *fen)
{
    // 1. Reset Meta-Game Flags
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

    // 2. Clear History Stacks
    ClearStack(state.undoStack);
    ClearStack(state.redoStack);

    // 3. Reset Dead Pieces
    deadWhiteCounter = 0;
    deadBlackCounter = 0;
    InitializeDeadPieces();

    // 4. Reset Visuals
    ResetJustMoved();
    UpdateLastMoveHighlight(-1, -1);
    ResetSelectedPiece();

    UnloadBoard();

    // 5. Reload Board
    // We make a copy because ReadFEN might modify the string or expects char*
    if (fen)
    {
        int len = strlen(fen);
        char *fenCopy = (char *)malloc(len + 1);
        if (fenCopy)
        {
            strcpy(fenCopy, fen);
            ReadFEN(fenCopy, len, false);
            free(fenCopy);

            // Adjust Turn because ResetsAndValidations flips it.
            // We want ResetsAndValidations to calculate moves for the CURRENT turn in FEN.
            // Since it flips the turn at the start, we must set it to the OPPONENT first.
            Turn = (Turn == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
            state.turn = Turn;
        }
    }

    // 6. Initial Validation
    ResetsAndValidations();
}

void RestartGame(void)
{
    LoadGameFromFEN(STARTING_FEN);
}
