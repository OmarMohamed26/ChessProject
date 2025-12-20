/*
 * draw.h
 *
 * Public drawing/layout API used by main.c.
 * - DrawBoard(theme): render board + pieces (call inside BeginDrawing/EndDrawing).
 * - LoadPiece(r,c,type,team,sq): load and assign a piece texture for a cell.
 * - ComputeSquareLength(): compute a consistent square size based on current window.
 *
 * Keep prototypes small and self-explanatory; implementation lives in draw.c.
 */
#ifndef DRAW_H
#define DRAW_H

#include "main.h"
#include <stdbool.h>

/* Render board and pieces for the provided color theme index. */
void DrawBoard(int ColorTheme, bool showFileRank);

/* Load a piece texture for cell (row,col). squareLength selects texture size. */
void LoadPiece(int row, int col, PieceType type, Team team);

/*Initialize the chess board to have appropriate starting values*/
void InitializeBoard(void);

/*Run after the game finishes or you want a new game to prevent memory leaks and flush the board*/
void UnloadBoard(void);

/*Highlight a single square*/
void HighlightSquare(int row, int col, int ColorTheme);

/*Highlights The piece when hovering over it*/
void HighlightHover(int ColorTheme);
/* Compute the pixel size of a single board square using current render dimensions. */
int ComputeSquareLength(void);

void HighlightValidMoves(bool selected);
// put reset validation in moves.h
/*Used in Make selected and last move borders*/
typedef struct SmartBorder
{
    int row, col;
    Rectangle rect;
} SmartBorder;

#endif