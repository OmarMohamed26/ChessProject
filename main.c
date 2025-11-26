/*
 * main.c
 *
 * Entry point: window setup, main loop, layout update and initial piece loading.
 *
 * Notes:
 * - Create the window (InitWindow) before calling ComputeSquareLength or loading
 *   resources that depend on render size.
 * - Recompute layout after resize (IsWindowResized) and update positions/textures
 *   as needed.
 * - GameBoard is the global board state (defined below) used by draw.c.
 */

#include <raylib.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "colors.h"
#include "draw.h"
#include "main.h"

/* Global board state accessible to draw.c and other modules. */
Cell GameBoard[8][8];

int main(void)
{
    // Initialize the game window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Chess");
    SetWindowMinSize(720, 720);

    SetTargetFPS(60);

    LoadPiece(0, 0, PIECE_KING, TEAM_WHITE, ComputeSquareLength());

    char firstTime = 1;

#ifdef DEBUG
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            printf("%d ", GameBoard[i][j].piece.type);
        }
        printf("\n");
    }
#endif

    while (!WindowShouldClose())
    {
        if (IsWindowResized())
            LoadPiece(0, 0, PIECE_KING, TEAM_WHITE, ComputeSquareLength());

        BeginDrawing();
        if (IsWindowResized() || firstTime)
        {
            ClearBackground(BACKGROUND);
            DrawBoard(THEME_BROWN);
            firstTime = 0;
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
