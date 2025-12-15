#ifndef MOVE_H
#define MOVE_H
#include "main.h"

void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);
void SetEmptyCell(Cell *cell);
void ValidateMoves(PieceType Piece, int CellX, int CellY, bool selected);
void MoveValidation(int CellX, int CellY, PieceType type, Team team, bool moved);
// Replaced all validation functions with one function
void ScanEnemyMoves();
void ResetValidation();
void ResetVulnerable();
bool HandleLinearSquare(int x, int y, Team team);
void HandlePawnMove(int CellX, int CellY, Team team, bool moved);
void HandleKnightSquare(int x, int y, Team team);
void HandleKnightMove(int CellX, int CellY, Team team);
void HandleKingMove(int CellX, int CellY, Team team);

//

#endif