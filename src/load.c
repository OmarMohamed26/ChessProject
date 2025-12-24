/**
 * load.c
 *
 * Responsibilities:
 * - Parse a FEN (Forsyth–Edwards Notation) piece placement string and populate
 *   the global GameBoard with the corresponding pieces using LoadPiece().
 * - Interpret digits as empty squares, '/' as rank separators, and letters as
 *   piece identifiers (uppercase = white, lowercase = black).
 *
 * Conventions / Notes:
 * - This parser treats the first FEN rank as rank 0 (top of the board). If the
 *   project's board origin differs (row 0 at bottom), adjust the rank mapping.
 * - Only piece placement is parsed; other FEN fields (active color, castling,
 *   en passant, halfmove/fullmove) are ignored by this function.
 * - Out-of-range files/ranks are clamped/ignored to avoid writes outside GameBoard.
 */

#include "draw.h"
#include "hash.h"
#include "main.h"
#include "settings.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>

static void ZeroRights(void);
static bool charInSequence(unsigned char chr);

/**
 * ReadFEN
 *
 * Parse the piece-placement portion of a FEN string and place pieces on GameBoard.
 *
 * Parameters:
 *  - FENstring : pointer to a NUL-terminated or length-limited FEN substring.
 *  - size      : maximum number of characters to read from FENstring.
 *  - testInputStringOnly: if true, validates the FEN string without modifying game state.
 *
 * Behavior:
 *  - Reads up to 'size' characters or until a NUL terminator is encountered.
 *  - Interprets digits '1'..'8' as that many consecutive empty squares (files).
 *  - '/' advances to the next rank (increments internal rank counter).
 *  - Letters map to piece types: p, r, n, b, q, k (case-insensitive). Uppercase
 *    letters are treated as TEAM_WHITE, lowercase as TEAM_BLACK.
 *  - Calls LoadPiece(row, col, PieceType, Team) for each piece placed.
 *
 * Safety:
 *  - If computed file index exceeds 8 it is clamped to 8 and parsing continues.
 *  - If computed row/col are outside [0,7], the character is skipped and parsing
 *    continues without writing to GameBoard.
 */
bool ReadFEN(const char *FENstring, size_t size, bool testInputStringOnly) // It returns true if this a valid or semi-valid FEN-string false otherwise
{
    int row;
    int col;
    int rank = 0; /* 0 = top rank (FEN first rank). Use 7 and decrement if your row0 is bottom */
    int file = 0; /* 0 = a-file (left) */

    size_t i;

    // --- 1. PIECE PLACEMENT ---
    for (i = 0; i < size && FENstring[i] != '\0' && FENstring[i] != ' '; i++)
    {
        unsigned char chr = FENstring[i];

        if (chr == '/')
        {
            rank++;
            file = 0;
            if (rank >= BOARD_SIZE)
            {
                break;
            }
            continue;
        }

        if (isdigit((unsigned char)chr))
        {
            file += chr - '0';
            if (file > BOARD_SIZE)
            {
                file = BOARD_SIZE;
            }
            continue;
        }

        if (!isalpha((unsigned char)chr))
        {
            return false;
        }

        Team color = islower(chr) ? TEAM_BLACK : TEAM_WHITE;
        unsigned char piece = tolower(chr);

        row = rank;
        col = file;

        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        {
            file++;
            continue;
        }

        // ONLY Load pieces if we are NOT just testing
        if (!testInputStringOnly)
        {
            switch (piece)
            {
            case 'r':
                LoadPiece(row, col, PIECE_ROOK, color, GAME_BOARD);
                break;
            case 'n':
                LoadPiece(row, col, PIECE_KNIGHT, color, GAME_BOARD);
                break;
            case 'b':
                LoadPiece(row, col, PIECE_BISHOP, color, GAME_BOARD);
                break;
            case 'q':
                LoadPiece(row, col, PIECE_QUEEN, color, GAME_BOARD);
                break;
            case 'k':
                LoadPiece(row, col, PIECE_KING, color, GAME_BOARD);
                break;
            case 'p':
                LoadPiece(row, col, PIECE_PAWN, color, GAME_BOARD);
                break;
            default:
                break;
            }
        }
        file++;
    }

    // --- 2. ACTIVE COLOR ---

    // Safe whitespace skip
    while (i < size && FENstring[i] != '\0' && isblank((unsigned char)FENstring[i]))
    {
        i++;
    }
    if (i >= size || FENstring[i] == '\0')
    {
        return false;
    }

    unsigned char chr = tolower((unsigned char)FENstring[i++]);
    if (chr != 'w' && chr != 'b')
    {
        return false;
    }

    if (!testInputStringOnly)
    {
        state.turn = (chr == 'w') ? TEAM_WHITE : TEAM_BLACK;
    }

    // Read the CastlingRights

    // FIX: Safe loop that doesn't overrun buffer or swallow the first char
    while (i < size && isblank((unsigned char)FENstring[i]))
    {
        i++;
    }
    if (i >= size)
    {
        return false;
    }
    chr = (unsigned char)FENstring[i++];

    if (chr != '-' && tolower(chr) != 'k' && tolower(chr) != 'q')
    {
        return false;
    }

    if (!testInputStringOnly)
    {
        ZeroRights();
    }

    if (chr == '-')
    {
        ; // Just leave it that way it's good enough
    }
    else
    {
        do
        {
            if (!testInputStringOnly) // <--- Added check
            {
                switch (chr)
                {
                case 'K':
                    state.whiteKingSide = true;
                    break;
                case 'k':
                    state.blackKingSide = true;
                    break;
                case 'Q':
                    state.whiteQueenSide = true;
                    break;
                case 'q':
                    state.blackQueenSide = true;
                    break;
                default:
                    break;
                }
            }

            // Check bounds and stop IF next char is space (don't consume the space yet)
            if (i >= size || FENstring[i] == '\0' || isblank((unsigned char)FENstring[i]))
            {
                break;
            }

            chr = (unsigned char)FENstring[i++];

        } while (true);
    }

    // FIX: Safe loop for En Passant
    while (i < size && isblank((unsigned char)FENstring[i]))
    {
        i++;
    }
    if (i >= size)
    {
        return false;
    }

    chr = (unsigned char)FENstring[i++];

    if (!charInSequence(chr))
    {
        return false;
    }

    if (chr == '-')
    {
        if (!testInputStringOnly)
        {
            state.enPassantCol = -1;
        }
    }
    else
    {
        int num = tolower(chr) - 'a';
        if (!testInputStringOnly)
        {
            state.enPassantCol = num;
        }

        if (i >= size)
        {
            return false;
        }

        chr = (unsigned char)FENstring[i++];
        num = chr - '0';

        if (num < 1 || num > BOARD_SIZE)
        {
            return false;
        }
    }

    // FIX: Use local variables so we don't corrupt state during validation
    int halfMove = 0;
    int fullMove = 1;

    if (sscanf(FENstring + i, "%d %d", &halfMove, &fullMove) != 2)
    {
        return false;
    }

    if (!testInputStringOnly) // <--- Added check
    {
        state.halfMoveClock = halfMove;
        state.fullMoveNumber = fullMove;
    }

    if (!testInputStringOnly)
    {
        if (state.DHA != NULL)
        {
            ClearDHA(state.DHA);
        }

        // Push the starting position
        PushDHA(state.DHA, CurrentGameStateHash());
    }
    return true;
}

/**
 * ZeroRights (static)
 *
 * Resets all castling rights flags in the global state to false.
 */
static void ZeroRights(void)
{
    state.whiteKingSide = false;
    state.whiteQueenSide = false;
    state.blackKingSide = false;
    state.blackQueenSide = false;
}

/**
 * charInSequence (static)
 *
 * Helper to validate if a character is a valid en passant target file or '-'.
 *
 * Parameters:
 *  - chr: The character to check.
 *
 * Returns:
 *  - true if chr is '-' or 'a'-'h' (case-insensitive).
 */
static bool charInSequence(unsigned char chr)
{
    // '-' or file letter a-h/A-H
    if (chr == '-')
    {
        return true;
    }
    unsigned char lower = tolower(chr);
    return (lower >= 'a' && lower <= 'h');
}
