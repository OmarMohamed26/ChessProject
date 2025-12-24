/**
 * utils.c
 *
 * Responsibilities:
 * - Provide high-level game management utilities.
 * - Handle game restarts and loading specific game states from FEN strings.
 * - Coordinate the resetting of various subsystems (board, stacks, visuals, flags)
 *   during state transitions.
 */

#include "utils.h"
#include "draw.h"
#include "load.h"
#include "main.h"
#include "move.h"
#include "settings.h"
#include "stack.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h> // For malloc/free
#include <string.h>

/**
 * LoadGameFromFEN
 *
 * Resets the entire game state and initializes the board based on a FEN string.
 *
 * Parameters:
 *  - fen: A null-terminated string containing the FEN record to load.
 *         If NULL, behavior is undefined (though likely safe due to checks in ReadFEN).
 *
 * Behavior:
 * 1. Resets meta-game flags (Checkmate, Stalemate, Promotion, etc.).
 * 2. Clears Undo/Redo history stacks.
 * 3. Resets dead piece counters and arrays.
 * 4. Resets visual state (highlights, selections).
 * 5. Unloads current textures and clears the board.
 * 6. Parses the FEN string to populate the board and game state (Turn, Castling, etc.).
 * 7. Runs initial validation (ResetsAndValidations) to calculate legal moves for the loaded state.
 *
 * Notes:
 * - Temporarily flips the turn before calling ResetsAndValidations because that function
 *   expects to flip the turn *before* calculating moves for the new player.
 */
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
        size_t len = strlen(fen);
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

/**
 * RestartGame
 *
 * Resets the game to the standard starting position.
 *
 * Behavior:
 * - Calls LoadGameFromFEN with the standard start FEN string defined in settings.h.
 */
void RestartGame(void)
{
    LoadGameFromFEN(STARTING_FEN);
}
