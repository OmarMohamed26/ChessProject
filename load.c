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
#include "main.h"
#include "settings.h"
#include <ctype.h>
#include <stddef.h>

// extern Cell GameBoard[BOARD_SIZE][BOARD_SIZE];

/**
 * ReadFEN
 *
 * Parse the piece-placement portion of a FEN string and place pieces on GameBoard.
 *
 * Parameters:
 *  - FENstring : pointer to a NUL-terminated or length-limited FEN substring.
 *  - size      : maximum number of characters to read from FENstring.
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
 *
 * Notes:
 *  - This function only writes piece placement; it does not clear or reset other
 *    board state (call InitializeBoard/UnloadBoard as appropriate before use).
 */
void ReadFEN(const char *FENstring, size_t size)
{
    int row;
    int col;
    int rank = 0; /* 0 = top rank (FEN first rank). Use 7 and decrement if your row0 is bottom */
    int file = 0; /* 0 = a-file (left) */

    for (size_t i = 0; i < size && FENstring[i] != '\0'; i++)
    {
        unsigned char ch = FENstring[i];

        if (ch == '/')
        {
            rank++;
            file = 0;
            if (rank >= BOARD_SIZE)
            {
                break;
            }
            continue;
        }

        if (isdigit((unsigned char)ch))
        {
            file += ch - '0';
            if (file > BOARD_SIZE)
            {
                file = BOARD_SIZE;
            }
            continue;
        }

        if (!isalpha((unsigned char)ch))
        {
            continue;
        }

        Team color = islower((unsigned char)ch) ? TEAM_BLACK : TEAM_WHITE;
        unsigned char piece = tolower(ch);

        row = rank;
        col = file;

        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        {
            file++;
            continue;
        }

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

        file++;
    }
}
