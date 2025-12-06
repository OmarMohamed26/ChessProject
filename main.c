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
#include "load.h"
#include <string.h>
#include "save.h"
#include <stdlib.h>

/* Global board state accessible to draw.c and other modules. */
Cell GameBoard[8][8];

int main(void)
{
    // Initialize the game window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Chess");
    SetWindowMinSize(480, 480);
    Image icon = LoadImage("assets/icon.png");
    SetWindowIcon(icon);
    InitializeBoard();

    char standard_game[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
    ReadFEN(standard_game, strlen(standard_game));

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

    char *savedGame = SaveFEN();
    SaveFileText("example.fen", savedGame);
    free(savedGame);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BACKGROUND);
        DrawBoard(THEME_BROWN);
        Highlight_Hover(THEME_BROWN);
        EndDrawing();
    }

    UnloadBoard();
    UnloadImage(icon);
    CloseWindow();

    return 0;
}