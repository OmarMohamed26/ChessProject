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
 * - Current implementation does not append side-to-move, castling, en-passant,
 *   halfmove clock or fullmove number fields. See TODOs below.
 *
 * FEN format used here (partial):
 * - Ranks are serialized from top (row 0) to bottom (row 7).
 * - Pieces: k,q,r,b,n,p with lowercase for black and uppercase for white.
 * - Empty squares are represented by digits 1-8. Ranks are separated by '/'.
 *
 * TODO:
 * - Add side to move (single 'w' or 'b' char) to the serialized string.
 * - Optionally support castling rights, en-passant, and move clocks if needed.
 */

#include "main.h"
#include <stdlib.h>
#include <raylib.h>
#include <ctype.h>
#include "save.h"

// TODO We must a add a small letter of the whole string to decide who is going to play next w or b

extern Cell GameBoard[8][8];

char *SaveFEN(void)
{
    // Big initial buffer size
    char *out = malloc(128 * sizeof(char));
    if (out == NULL)
    {
        TraceLog(LOG_WARNING, "Couldn't allocate space for the FEN string");
        return NULL;
    }

    int consecutiveEmptyCells = 0;
    char currentPiece;
    int counter = 0;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
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
        if (row < 7)
        {
            out[counter++] = '/';
        }
    }

    /* ensure space for NUL terminator */
    char *temp = realloc(out, counter + 1);
    if (temp == NULL)
    {
        free(out);
        TraceLog(LOG_WARNING, "Failed to resize the buffer");
        return NULL;
    }

    out = temp;
    out[counter] = '\0';

    return out;
}
