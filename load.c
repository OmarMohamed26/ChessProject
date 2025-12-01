#include "main.h"
#include "draw.h"
#include <ctype.h>

extern Cell GameBoard[8][8];

void ReadFEN(const char *FENstring, int size)
{
    int row, col;
    int rank = 0; /* 0 = top rank (FEN first rank). Use 7 and decrement if your row0 is bottom */
    int file = 0; /* 0 = a-file (left) */

    for (int i = 0; i < size && FENstring[i] != '\0'; i++)
    {
        char ch = FENstring[i];

        if (ch == '/')
        {
            rank++;
            file = 0;
            if (rank >= 8)
                break;
            continue;
        }

        if (isdigit((unsigned char)ch))
        {
            file += ch - '0';
            if (file > 8)
                file = 8;
            continue;
        }

        if (!isalpha((unsigned char)ch))
            continue;

        Team color = islower((unsigned char)ch) ? TEAM_BLACK : TEAM_WHITE;
        char piece = tolower((unsigned char)ch);

        row = rank;
        col = file;

        if (row < 0 || row >= 8 || col < 0 || col >= 8)
        {
            file++;
            continue;
        }

        switch (piece)
        {
        case 'r':
            LoadPiece(row, col, PIECE_ROOK, color);
            break;
        case 'n':
            LoadPiece(row, col, PIECE_KNIGHT, color);
            break;
        case 'b':
            LoadPiece(row, col, PIECE_BISHOP, color);
            break;
        case 'q':
            LoadPiece(row, col, PIECE_QUEEN, color);
            break;
        case 'k':
            LoadPiece(row, col, PIECE_KING, color);
            break;
        case 'p':
            LoadPiece(row, col, PIECE_PAWN, color);
            break;
        default:
            break;
        }

        file++;
    }
}
