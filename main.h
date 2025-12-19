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
    Texture2D texture; /* put large/aligned field first to avoid padding gaps */
    PieceType type;    /* enum (usually 4 bytes) */
    Team team;         /* enum (usually 4 bytes) */
    char hasMoved;     /* single byte; now sits after other 4-byte fields */
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
    Piece piece;           /* piece occupying the cell (PIECE_NONE if empty) */
    Vector2 pos;           /* pixel position for rendering (top-left) */
    int row, col;          /* board coordinates (0..7) */
    bool primaryValid : 1; // This is a primary validation geometrically
    bool isvalid : 1;      // Final validation of moves FINAL_VALIDATION
    bool selected : 1;     // will also need this
    bool vulnerable : 1;   // what pieces are under attack on my team this will help in EASY_MODE
} Cell;

typedef struct Player
{
    Team team;
    bool Checked : 1;
    bool Checkmated : 1;
    bool SimChecked : 1;
    bool Stalemate : 1;
} Player;

// typedef struct
// {
// row col(init/final) piece team

// } Move;

extern int pointer;
extern Team Turn;
extern bool Checkmate, Stalemate;
#endif /* MAIN_H */