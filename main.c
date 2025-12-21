/*
 * main.c
 *
 * Entry point: window setup, main loop, layout update and initial piece loading.
 *
 * Notes:
 * - Create the window (InitWindow) before calling ComputeSquareLength or loading
 * - GameBoard is the global board state (defined below) used by draw.c.
 */

#ifdef DEBUG
#include <stdio.h>
#endif
#include "colors.h"
#include "draw.h"
#include "load.h"
#include "main.h"
#include "raylib.h"
#include "save.h"
#include "settings.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Global board state accessible to draw.c and other modules. */
Cell GameBoard[BOARD_SIZE][BOARD_SIZE];
Player Player1, Player2;
Team Turn = TEAM_WHITE;
bool Checkmate = false;
bool Stalemate = false;

int main(void)
{
    Player1.team = TEAM_WHITE;
    Player2.team = TEAM_BLACK;
    // Initialize the game window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#ifdef DEBUG
    SetTraceLogLevel(LOG_DEBUG);
#endif
    InitWindow(START_SCREEN_WIDTH, START_SCREEN_HEIGHT, "Chess");
    SetWindowMinSize(MIN_SCREEN_WIDTH, MIN_SCREEN_WIDTH);
    Image icon = LoadImage("assets/icon.png");
    SetWindowIcon(icon);
    SetTargetFPS(FPS);
    InitializeBoard();

    // char standard_game[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
    char standard_game[] = "8/8/8/8/8/8/8/R1N1K2k";
    ReadFEN(standard_game, strlen(standard_game));

    bool showFps = false;
    bool showFileRank = true;

#ifdef DEBUG
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
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
        if (IsKeyPressed(KEY_F5))
        {
            showFps = !showFps;
        }

        if (IsKeyPressed(KEY_R))
        {
            showFileRank = !showFileRank;
        }

        BeginDrawing();
        ClearBackground(BACKGROUND);

        ColorTheme theme = THEME_BROWN;

        DrawBoard(theme, showFileRank);
        HighlightHover(theme);
        if (Player1.Checked)
        {
            DrawText("WHITE IS CHECKED!", 20, 20, 30, BLACK);
        }
        if (Player2.Checked)
        {
            DrawText("BLACK IS CHECKED!", GetRenderWidth() - 340, 20, 30, BLACK);
        }
        if (Checkmate)
        {
            DrawText("CHECKMATE", GetRenderWidth() / 2 - 140, 30, 30, RED);
        }
        if (Stalemate)
        {
            DrawText("STALEMATE", GetRenderWidth() / 2 - 140, 20, 30, GRAY);
        }

        if (showFps)
        {
            DrawFPS(0, 0);
        }
        EndDrawing();
    }

    UnloadBoard();
    UnloadImage(icon);
    CloseWindow();

    return 0;
}
