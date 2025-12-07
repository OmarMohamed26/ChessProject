/*
 * main.h
 *
 * Core game types used across the project.
 * Keep this header small and self-contained so other modules can include it.
 */

#ifndef MAIN_H
#define MAIN_H

#include <raylib.h>

/* PieceType
 * - PIECE_NONE == 0 so zero-initialized memory means "empty cell".
 */
typedef enum
{
    PIECE_NONE = 0, /* empty square */
    PIECE_KING,
    PIECE_QUEEN,
    PIECE_ROOK,
    PIECE_BISHOP,
    PIECE_KNIGHT,
    PIECE_PAWN
} PieceType;

/* Team (side/color) */
typedef enum
{
    TEAM_WHITE = 0,
    TEAM_BLACK
} Team;

/* Piece
 * Represents a single chess piece and its small state.
 *
 * Notes:
 * - `type` and `team` identify the piece.
 * - `hasMoved` / `enPassant` are small flags (0/1).
 * - `spriteId` is an optional index you can use for atlases.
 * - `texture` is a GPU texture (Texture2D). The module that replaces a texture
 *   is responsible for UnloadTexture() on the old texture to avoid GPU leaks.
 */
typedef struct Piece
{
    PieceType type;     /* piece kind (PIECE_NONE = empty) */
    Team team;          /* TEAM_WHITE / TEAM_BLACK */
    char hasMoved : 1;  /* boolean: moved before (castling/pawn) */
    char enPassant : 1; /* boolean: pawn is vulnerable to en-passant */
    Texture2D texture;  /* GPU texture; texture.id == 0 means "no texture" */
} Piece;

/* Cell
 * Single board square with logical coords, render position, and stored piece.
 *
 * - row/col: board indices (0..7)
 * - pos: top-left pixel position for drawing (use DrawTexturePro to scale)
 * - piece: content of the square
 */
typedef struct Cell
{
    int row, col; /* board coordinates (0..7) */
    Vector2 pos;  /* pixel position for rendering (top-left) */
    Piece piece;  /* piece occupying the cell (PIECE_NONE if empty) */
} Cell;

extern int pointer;
#endif /* MAIN_H */