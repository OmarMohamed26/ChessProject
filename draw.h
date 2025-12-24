/*
 * draw.h
 *
 * Public drawing/layout API used by main.c.
 * - DrawBoard(theme): render board + pieces (call inside BeginDrawing/EndDrawing).
 * - LoadPiece(r,c,type,team, place): load and assign a piece texture for a cell.
 * - ComputeSquareLength(): compute a consistent square size based on current window.
 *
 * Keep prototypes small and self-explanatory; implementation lives in draw.c.
 */
#ifndef DRAW_H
#define DRAW_H

#include "main.h"
#include <stdbool.h>

typedef enum LoadPlace
{
    GAME_BOARD = 0,
    DEAD_WHITE_PIECES,
    DEAD_BLACK_PIECES,
} LoadPlace;

/* Render board and pieces for the provided color theme index. */
void DrawBoard(int ColorTheme, bool showFileRank);

/* Load a piece texture for cell (row,col). squareLength selects texture size. */
void LoadPiece(int row, int col, PieceType type, Team team, LoadPlace place);

/* Initialize the chess board to have appropriate starting values */
void InitializeBoard(void);

/* Initialize the DeadPieces to have appropriate starting values */
void InitializeDeadPieces(void);

/* Run after the game finishes or you want a new game to prevent memory leaks and flush the board */
void UnloadBoard(void);

/* Run after the game finishes or you want a new game to prevent memory leaks and flush the DeadPieces */
void UnloadDeadPieces(void);

/* Highlight a single square */
void HighlightSquare(int row, int col, int ColorTheme);

/* Highlights The piece when hovering over it */
void HighlightHover(int ColorTheme);

/* Compute the pixel size of a single board square using current render dimensions. */
int ComputeSquareLength(void);

/* Highlight valid moves for the selected piece */
void HighlightValidMoves(bool selected);

void UpdateLastMoveHighlight(int row, int col);

void DrawDebugInfo(void);

/* NEW: Draws the game status (Check, Mate, Draw, etc.) */
void DrawGameStatus(void);

#endif /* DRAW_H */