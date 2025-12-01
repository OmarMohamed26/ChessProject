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

/* Render board and pieces for the provided color theme index. */
void DrawBoard(int ColorTheme);

/* Load a piece texture for cell (row,col). squareLength selects texture size. */
void LoadPiece(int row, int col, PieceType type, Team team);

/* Compute the pixel size of a single board square using current render dimensions. */
int ComputeSquareLength(void);

#endif