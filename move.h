#ifndef MOVE_H
#define MOVE_H
#include "main.h"

void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);
void SetEmptyCell(Cell *cell);
void ValidateAndDevalidateMoves(PieceType Piece, int CellX, int CellY, int ColorTheme, bool selected);
#endif

// Validation functions for each piece
void KingValidation(int CellX, int CellY, int ColorTheme);
void QueenValidation(int CellX, int CellY, int ColorTheme);
void RookValidation(int CellX, int CellY, int ColorTheme);
void BishopValidation(int CellX, int CellY, int ColorTheme);
void KnightValidation(int CellX, int CellY, int ColorTheme);
void PawnValidation(int CellX, int CellY, int ColorTheme);
void KingDevalidation(int CellX, int CellY, int ColorTheme);
void QueenDevalidation(int CellX, int CellY, int ColorTheme);
void RookDevalidation(int CellX, int CellY, int ColorTheme);
void BishopDevalidation(int CellX, int CellY, int ColorTheme);
void KnightDevalidation(int CellX, int CellY, int ColorTheme);
void PawnDevalidation(int CellX, int CellY, int ColorTheme);