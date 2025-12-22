/*
 * main.c
 *
 * Entry point: window setup, main loop, layout update and initial piece loading.
 *
 * Notes:
 * - Create the window (InitWindow) before calling ComputeSquareLength or loading
 * - GameBoard is the global board state (defined below) used by draw.c.
 */

#define MAIN_C
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

GameState state;

int main(void)
{
    state.deadWhiteCounter = 0;
    state.deadBlackCounter = 0;
    state.whitePlayer.team = TEAM_WHITE;
    state.blackPlayer.team = TEAM_BLACK;
    state.turn = TEAM_WHITE;
    state.isCheckmate = false;
    state.isStalemate = false;

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
    InitializeDeadPieces();

    char standard_game[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
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

    for (int i = 0; i < 2 * BOARD_SIZE; i++)
    {
        printf("%d ", DeadWhitePieces[i].piece.type);
    }
#endif

    unsigned char *savedGame = SaveFEN();
    SaveFileText("example.fen", (const char *)savedGame);
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
        if (state.whitePlayer.Checked)
        {
            DrawText("WHITE IS CHECKED!", 20, 20, 30, BLACK);
        }
        if (state.blackPlayer.Checked)
        {
            DrawText("BLACK IS CHECKED!", GetRenderWidth() - 340, 20, 30, BLACK);
        }
        if (state.isCheckmate)
        {
            DrawText("CHECKMATE", GetRenderWidth() / 2 - 140, 30, 30, RED);
        }
        if (state.isStalemate)
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
    UnloadDeadPieces();
    UnloadImage(icon);
    CloseWindow();

    return 0;
}
