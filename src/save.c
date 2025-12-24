/*
 * save.c
 *
 * Functions to serialize the current board state into a FEN-like string.
 *
 * Notes:
 * - The board is read from the external GameBoard[8][8] variable (declared main.c).
 * - SaveFEN allocates a heap buffer containing a FEN-like representation of the board.
 *   The caller is responsible for freeing the returned buffer with free().
 * - On allocation failure or invalid piece type the function returns NULL.
 *
 * FEN format used here:
 * - Ranks are serialized from top (row 0) to bottom (row 7).
 * - Pieces: k,q,r,b,n,p with lowercase for black and uppercase for white.
 * - Empty squares are represented by digits 1-8. Ranks are separated by '/'.
 *
 */

#include "save.h"
#include "main.h"
#include "raylib.h"
#include "settings.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char *SaveFEN(void)
{
    // Big initial buffer size
    unsigned char *out = malloc((MAX_FEN_BUFFER_SIZE + 1) * sizeof(unsigned char));
    if (out == NULL)
    {
        TraceLog(LOG_WARNING, "Couldn't allocate space for the FEN string");
        return NULL;
    }

    int consecutiveEmptyCells = 0;
    unsigned char currentPiece;
    int counter = 0;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            PieceType type = GameBoard[row][col].piece.type;
            Team team = GameBoard[row][col].piece.team;

            if (type != PIECE_NONE)
            {
                if (consecutiveEmptyCells != 0)
                {
                    out[counter++] = '0' + consecutiveEmptyCells;
                    consecutiveEmptyCells = 0; /* reset after flushing */
                }

                switch (type)
                {
                case PIECE_BISHOP:
                    currentPiece = 'b';
                    break;
                case PIECE_KING:
                    currentPiece = 'k';
                    break;
                case PIECE_KNIGHT:
                    currentPiece = 'n';
                    break;
                case PIECE_PAWN:
                    currentPiece = 'p';
                    break;
                case PIECE_QUEEN:
                    currentPiece = 'q';
                    break;
                case PIECE_ROOK:
                    currentPiece = 'r';
                    break;
                default:
                    TraceLog(LOG_WARNING, "invalid character %c for piece type", type);
                    free(out);
                    return NULL;
                }

                currentPiece = (team == TEAM_BLACK ? currentPiece : toupper(currentPiece));

                out[counter++] = currentPiece;
            }
            else
            {
                consecutiveEmptyCells++;
            }
        }

        /* flush any remaining empty count at end of rank */
        if (consecutiveEmptyCells != 0)
        {
            out[counter++] = '0' + consecutiveEmptyCells;
            consecutiveEmptyCells = 0;
        }

        /* add rank separator except after the last rank */
        if (row < BOARD_SIZE - 1)
        {
            out[counter++] = '/';
        }
    }

    // Active color

    out[counter++] = ' '; // leave an empty space
    out[counter++] = (state.turn == TEAM_WHITE) ? 'w' : 'b';

    // CastleRights

    out[counter++] = ' '; // leave an empty space

    bool noRights = true;

    if (state.whiteKingSide)
    {
        out[counter++] = 'K';
        noRights = false;
    }

    if (state.whiteQueenSide)
    {
        out[counter++] = 'Q';
        noRights = false;
    }

    if (state.blackKingSide)
    {
        out[counter++] = 'k';
        noRights = false;
    }

    if (state.blackQueenSide)
    {
        out[counter++] = 'q';
        noRights = false;
    }

    if (noRights)
    {
        out[counter++] = '-';
    }

    // EnPassant

    out[counter++] = ' '; // leave an empty space

    if (state.enPassantCol != -1)
    {
        out[counter++] = 'a' + state.enPassantCol;

        if (state.turn == TEAM_WHITE)
        {
            out[counter++] = '6';
        }
        else
        {
            out[counter++] = '3';
        }
    }
    else
    {
        out[counter++] = '-';
    }
    // halfMove counter

    out[counter++] = ' '; // leave an empty space

    counter += snprintf((char *)out + counter, (2 * MAX_HALF_FULL_MOVE_DIGITS) + 1, "%d %d", state.halfMoveClock, state.fullMoveNumber);

    /* ensure space for NUL terminator */
    unsigned char *temp = realloc(out, counter + 1);
    if (temp == NULL)
    {
        TraceLog(LOG_WARNING, "Failed to resize the buffer");
        out[counter] = '\0';
        return out;
    }

    out = temp;
    out[counter] = '\0';

    return out;
}
