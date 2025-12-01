#include "main.h"
#include "draw.h"
#include "move.h"

extern Cell GameBoard[8][8];

void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol)
{
    LoadPiece(finalRow, finalCol, GameBoard[initialRow][initialCol].piece.type, GameBoard[initialRow][initialCol].piece.team);
    GameBoard[finalRow][finalCol].piece.hasMoved = 1;
    SetEmptyCell(&GameBoard[initialRow][initialCol]);
}

void SetEmptyCell(Cell *cell)
{
    cell->piece.type = PIECE_NONE;
    cell->piece.hasMoved = 0;
    cell->piece.enPassant = 0;
    cell->piece.team = TEAM_WHITE;
    if (cell->piece.texture.id != 0)
        UnloadTexture(cell->piece.texture);
}
