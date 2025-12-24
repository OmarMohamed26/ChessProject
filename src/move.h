/**
 * move.h
 *
 * Responsibilities:
 * - Export functions for moving pieces, validating moves, and managing game state.
 * - Includes prototypes for move execution, validation, simulation, and undo/redo.
 */

#ifndef MOVE_H
#define MOVE_H

#include "main.h"

/* Executes a move on the board, handling captures and special rules */
void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);

/* Clears a cell and unloads its texture */
void SetEmptyCell(Cell *cell);

/* Computes raw geometric moves for a piece (primary validation) */
void PrimaryValidation(PieceType Piece, int CellX, int CellY, bool selected);

/* Helper for geometric validation of specific piece types */
void MoveValidation(int CellX, int CellY, PieceType type, Team team, bool moved);

/* Filters primary moves to ensure they don't leave the King in check */
void FinalValidation(int CellX, int CellY, bool selected);

/* Scans all enemy pieces to mark squares they attack (vulnerable) */
void ScanEnemyMoves();

/* Checks if the current player's King is under attack */
void CheckValidation();

/* Checks if a simulated move results in the King being under attack */
void SimCheckValidation();

/* Resets the 'isvalid' flag for all cells */
void ResetValidation();

/* Resets the 'vulnerable' flag for all cells */
void ResetVulnerable();

/* Helper for sliding pieces (Rook, Bishop, Queen) */
bool HandleLinearSquare(int row, int col, Team team);

/* Helper for Pawn movement logic */
void HandlePawnMove(int CellX, int CellY, Team team, bool moved);

/* Helper for Knight movement logic (single square) */
void HandleKnightSquare(int row, int col, Team team);

/* Helper for Knight movement logic (all 8 squares) */
void HandleKnightMove(int CellX, int CellY, Team team);

/* Helper for King movement logic */
void HandleKingMove(int CellX, int CellY, Team team);

/* Simulates a move on the board (without graphics/sound) */
void MoveSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece);

/* Reverts a simulated move */
void UndoSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece1, PieceType piece2, Team team2);

/* Checks if a player has any legal moves to escape check */
bool CheckmateFlagCheck(Team playerTeam);

/* Validates if the game is in Checkmate */
void CheckmateValidation();

/* Validates if the game is in Stalemate */
void StalemateValidation();

/* Scans all friendly pieces (unused) */
void ScanFriendlyMoves();

/* Resets the 'primaryValid' flag for all cells */
void ResetPrimaryValidation();

/* Central routine to update game state after a move (turn flip, checks, etc.) */
void ResetsAndValidations();

/* Resets the 'hasMoved' flag for all pieces */
void ResetMovedStatus();

/* Promotes a pawn to the selected piece type */
void PromotePawn(PieceType selectedType);

/* Validates Castling moves */
void PrimaryCastlingValidation();

/* Validates En Passant moves */
void PrimaryEnpassantValidation(int row, int col);

/* Resets the 'JustMoved' flag (used for En Passant) */
void ResetJustMoved();

/* Undoes the last move */
void UndoMove(void);

/*Redo the last move*/
void RedoMove(void);

#endif