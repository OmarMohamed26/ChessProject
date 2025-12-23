#ifndef MOVE_H
#define MOVE_H

#include "main.h"

void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);
void SetEmptyCell(Cell *cell);
void PrimaryValidation(PieceType Piece, int CellX, int CellY, bool selected);
void MoveValidation(int CellX, int CellY, PieceType type, Team team, bool moved);
void FinalValidation(int CellX, int CellY, bool selected);
// Replaced all validation functions with one function
void ScanEnemyMoves();
void CheckValidation();
void SimCheckValidation();
void ResetValidation();
void ResetVulnerable();
bool HandleLinearSquare(int row, int col, Team team);
void HandlePawnMove(int CellX, int CellY, Team team, bool moved);
void HandleKnightSquare(int row, int col, Team team);
void HandleKnightMove(int CellX, int CellY, Team team);
void HandleKingMove(int CellX, int CellY, Team team);
void MoveSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece);
void UndoSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece1, PieceType piece2, Team team2);
bool CheckmateFlagCheck(Team playerTeam);
void CheckmateValidation();
void StalemateValidation();
void ScanFriendlyMoves();
void ResetPrimaryValidation();
void ResetsAndValidations();
void ResetMovedStatus();
void PromotePawn(PieceType selectedType);
void PrimaryCastlingValidation();

#endif