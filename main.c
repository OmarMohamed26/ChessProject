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
#include "hash.h"
#include "load.h"
#include "main.h"
#include "move.h"
#include "raylib.h"
#include "save.h"
#include "settings.h"
#include "stack.h"
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
    state.isCheckmate = false;
    state.isStalemate = false;
    state.isPromoting = false;
    state.promotionRow = -1;
    state.promotionCol = -1;
    state.isRepeated3times = false;

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

    state.DHA = InitializeDHA(INITIAL_DYNAMIC_HASH_ARRAY_SIZE);
    state.undoStack = InitializeStack(INITAL_UNDO_REDO_STACK_SIZE);
    state.redoStack = InitializeStack(INITAL_UNDO_REDO_STACK_SIZE);

    char standard_game[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    // it should look like this rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    // Pieces Turn CastlingRights(or -) EnPassant(could look like e3) halfMoveClock fullMoveClock(after black moves)
    ReadFEN(standard_game, strlen(standard_game), false);

    bool showDebugMenu = false;
    bool showFileRank = true;

#ifdef DEBUG
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            printf("%d ", state.board[i][j].piece.type);
        }
        printf("\n");
    }

    for (int i = 0; i < 2 * BOARD_SIZE; i++)
    {
        printf("%d ", state.DeadWhitePieces[i].piece.type);
    }
#endif

    unsigned char *savedGame = SaveFEN();
    SaveFileText("example.fen", (const char *)savedGame);
    free(savedGame);

    while (!WindowShouldClose())
    {
        // Keyboard responses

        if (IsKeyPressed(KEY_F5))
        {
            showDebugMenu = !showDebugMenu;
        }

        if (IsKeyPressed(KEY_R))
        {
            showFileRank = !showFileRank;
        }

        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z))
            {
                RedoMove();
            }
            else if (IsKeyPressed(KEY_Z))
            {
                UndoMove();
            }
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
        else if (state.blackPlayer.Checked)
        {
            DrawText("BLACK IS CHECKED!", GetRenderWidth() - 340, 20, 30, BLACK);
        }

        if (state.isCheckmate)
        {
            DrawText("CHECKMATE", GetRenderWidth() / 2 - 140, 30, 30, RED);
        }
        else if (state.isStalemate)
        {
            DrawText("STALEMATE", GetRenderWidth() / 2 - 140, 20, 30, GRAY);
        }
        // NEW: Check the flag you set in move.c
        else if (state.isRepeated3times)
        {
            DrawText("DRAW (REPETITION)", GetRenderWidth() / 2 - 160, 70, 30, BLUE);
        }
        // NEW: Draw UI
        else if (state.isInsufficientMaterial)
        {
            DrawText("DRAW (INSUFFICIENT MATERIAL)", GetRenderWidth() / 2 - 220, 70, 30, BLUE);
        }
        else if (state.halfMoveClock >= 100)
        {
            DrawText("DRAW (50 MOVES)", GetRenderWidth() / 2 - 160, 70, 30, BLUE);
        }

        if (showDebugMenu)
        {
            DrawDebugInfo();
        }

        EndDrawing();
    }

    UnloadBoard();
    FreeDHA(state.DHA);
    FreeStack(state.undoStack);
    FreeStack(state.redoStack);
    UnloadDeadPieces();
    UnloadImage(icon);
    CloseWindow();

    return 0;
}
