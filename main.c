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
#define RAYGUI_IMPLEMENTATION

#ifdef DEBUG
#include <stdio.h>
#endif
#include "colors.h"
#include "draw.h"
#include "hash.h"
#include "load.h"
#include "main.h"
#include "move.h"
#include "raygui.h"
#include "raylib.h"
#include "save.h"
#include "settings.h"
#include "stack.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Local state for the Save Game UI
static bool showSaveTextInput = false;
static bool showOverwriteDialog = false;
static char saveFileName[MAX_FILE_NAME_LENGTH] = {0};

// NEW: Local state for Load Game UI
static bool showLoadFileDialog = false;
static int loadFileScrollIndex = 0;
static int loadFileActiveIndex = -1;
// TODO add infinite string buffer
static char loadFileListBuffer[MAX_LOAD_FILES_TOTAL_LENGTH] = {0}; // Buffer to hold "file1.fen;file2.fen;..."
static FilePathList loadFilePaths = {0};                           // Raylib struct to hold directory info

// State initialization
GameState state;

int main(void)
{
    // State initialization
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
    state.isInputLocked = false; // Initialize the lock

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
    InitAudioDevice();

    // Initialize the Game
    InitializeBoard();
    InitializeDeadPieces();

    state.DHA = InitializeDHA(INITIAL_DYNAMIC_HASH_ARRAY_SIZE);
    state.undoStack = InitializeStack(INITAL_UNDO_REDO_STACK_SIZE);
    state.redoStack = InitializeStack(INITAL_UNDO_REDO_STACK_SIZE);

    char standard_game[] = STARTING_FEN;
    // it should look like this rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    // Pieces Turn CastlingRights(or -) EnPassant(could look like e3) halfMoveClock fullMoveClock(after black moves)
    ReadFEN(standard_game, strlen(standard_game), false);

    bool showDebugMenu = false;
    bool showFileRank = true;

    // sound effects
    // CHANGED: Load sounds into state
    state.sounds.capture = LoadSound("assets/sound/Capture.mp3");
    state.sounds.check = LoadSound("assets/sound/Check.mp3");
    state.sounds.checkMate = LoadSound("assets/sound/Checkmate.mp3");
    state.sounds.move = LoadSound("assets/sound/Move.mp3");

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

        // --- REPLACED OLD UI BLOCK WITH THIS ---
        DrawGameStatus();
        // ---------------------------------------

        // NEW: Draw GUI Buttons
        HandleGui();

        if (showDebugMenu)
        {
            DrawDebugInfo();
        }

        EndDrawing();
    }

    // Deinitialize and Free Memory

    UnloadBoard();

    FreeDHA(state.DHA);

    FreeStack(state.undoStack);
    FreeStack(state.redoStack);

    UnloadDeadPieces();

    UnloadImage(icon);

    UnloadSound(state.sounds.capture);
    UnloadSound(state.sounds.check);
    UnloadSound(state.sounds.checkMate);
    UnloadSound(state.sounds.move);

    CloseAudioDevice();

    CloseWindow();

    return 0;
}

void HandleGui(void)
{
    // --- BUTTON 0: RESTART ---
    if (GuiButton(GetTopButtonRect(0), GuiIconText(ICON_RESTART, "Restart")))
    {
        RestartGame();
    }

    // --- BUTTON 1: SAVE GAME ---
    if (GuiButton(GetTopButtonRect(1), GuiIconText(ICON_FILE_SAVE, "Save")))
    {
        showSaveTextInput = true;
        state.isInputLocked = true; // Freeze board
        // this is kind of simillar to the standard <string.h> way
        TextCopy(saveFileName, ""); // Clear previous input
    }

    // --- BUTTON 2: LOAD GAME ---
    if (GuiButton(GetTopButtonRect(2), GuiIconText(ICON_FILE_OPEN, "Load")))
    {
        showLoadFileDialog = true;
        state.isInputLocked = true;
        loadFileActiveIndex = -1;
        loadFileScrollIndex = 0;

        // 1. Load files from "saves/" directory
        if (DirectoryExists("saves"))
        {
            // Reload the folder
            if (loadFilePaths.count > 0)
            {
                UnloadDirectoryFiles(loadFilePaths);
            }
            loadFilePaths = LoadDirectoryFiles("saves");

            // 2. Format string for GuiListView (items separated by semicolons)
            // e.g., "game1.fen;game2.fen;cool_save.fen"
            memset(loadFileListBuffer, 0, sizeof(loadFileListBuffer));

            for (size_t i = 0; i < loadFilePaths.count; i++)
            {
                // Only show .fen files
                if (IsFileExtension(loadFilePaths.paths[i], ".fen"))
                {
                    const char *fileName = GetFileName(loadFilePaths.paths[i]);
                    strcat(loadFileListBuffer, fileName);

                    /* Add a semicolon separator between file names for GuiListView
                       (GuiListView expects items separated by ';'), but avoid adding
                       a trailing semicolon after the last item. */
                    if (i < loadFilePaths.count - 1)
                    {
                        strcat(loadFileListBuffer, ";");
                    }
                }
            }
        }
        else
        {
            TextCopy(loadFileListBuffer, "No saves directory found");
        }
    }

    // --- POPUP 1: TEXT INPUT (Filename) ---
    if (showSaveTextInput)
    {
        GuiUnlock(); // Ensure we can interact with the popup

        // A little fade effect that applies to the hole screen to make it clear that we are in pop up mode
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

        // result  1 means that the Save button was pressed , 0 means cancel and 2 means close
        int result = GuiTextInputBox(
            (Rectangle){
                (float)GetScreenWidth() / 2 - (POPUP_INPUT_WIDTH / 2),
                (float)GetScreenHeight() / 2 - (POPUP_INPUT_HEIGHT / 2),
                POPUP_INPUT_WIDTH,
                POPUP_INPUT_HEIGHT},
            GuiIconText(ICON_FILE_SAVE, "Save Game"),
            "Enter file name (without .fen):",
            "Save;Cancel",
            saveFileName,
            MAX_FILE_NAME_LENGTH,
            NULL);

        if (result == 1) // Save clicked
        {
            // Check if file exists
            char fullPath[MAX_FILE_NAME_LENGTH + 6 /*for the saves part of the of the path*/];
            TextCopy(fullPath, TextFormat("saves/%s.fen", saveFileName));

            if (FileExists(fullPath))
            {
                // File exists, switch to overwrite dialog
                showSaveTextInput = false;
                showOverwriteDialog = true;
            }
            else
            {
                // File doesn't exist, save immediately
                unsigned char *fenString = SaveFEN();
                SaveFileText(fullPath, (char *)fenString);
                free(fenString);

                showSaveTextInput = false;
                state.isInputLocked = false; // Unfreeze
            }
        }
        else if (result == 0 || result == 2) // Cancel or Close
        {
            showSaveTextInput = false;
            state.isInputLocked = false; // Unfreeze
        }
    }

    // --- POPUP 2: OVERWRITE CONFIRMATION ---
    if (showOverwriteDialog)
    {
        GuiUnlock();
        // A little fade effect that applies to the hole screen to make it clear that we are in pop up mode
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

        int result = GuiMessageBox(
            (Rectangle){
                (float)GetScreenWidth() / 2 - (POPUP_OVERWRITE_WIDTH / 2),
                (float)GetScreenHeight() / 2 - (POPUP_OVERWRITE_HEIGHT / 2),
                POPUP_OVERWRITE_WIDTH,
                POPUP_OVERWRITE_HEIGHT},
            GuiIconText(ICON_WARNING, "File Exists"),
            "File already exists. Overwrite?",
            "Yes;No");

        if (result == 1) // Yes
        {
            char fullPath[MAX_FILE_NAME_LENGTH + 6];
            TextCopy(fullPath, TextFormat("saves/%s.fen", saveFileName));

            unsigned char *fenString = SaveFEN();
            SaveFileText(fullPath, (char *)fenString);
            free(fenString);

            showOverwriteDialog = false;
            state.isInputLocked = false; // Unfreeze
        }
        else if (result == 0) // No
        {
            showOverwriteDialog = false;
            state.isInputLocked = false;
        }
    }

    // --- POPUP 3: LOAD GAME DIALOG ---
    if (showLoadFileDialog)
    {
        GuiUnlock();
        // A little fade effect that applies to the hole screen to make it clear that we are in pop up mode
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

        // Calculate centered position
        Rectangle winRect = {
            (float)GetScreenWidth() / 2 - (POPUP_LOAD_WIDTH / 2),
            (float)GetScreenHeight() / 2 - (POPUP_LOAD_HEIGHT / 2),
            POPUP_LOAD_WIDTH,
            POPUP_LOAD_HEIGHT};

        // Window Box
        if (GuiWindowBox(winRect, "Load Game"))
        {
            showLoadFileDialog = false;
            state.isInputLocked = false;
        }

        // File List View
        Rectangle listRect = {winRect.x + 10, winRect.y + 30, winRect.width - 20, winRect.height - 80};
        GuiListView(listRect, loadFileListBuffer, &loadFileScrollIndex, &loadFileActiveIndex);

        // Load Button
        if (GuiButton((Rectangle){winRect.x + 10, winRect.y + winRect.height - 40, 80, 30}, GuiIconText(ICON_FILE_OPEN, "Load")))
        {
            if (loadFileActiveIndex >= 0 && loadFileActiveIndex < (int)loadFilePaths.count)
            {
                // 1. Construct full path
                // Note: GuiListView index matches the filtered list.
                // For simplicity here assuming all files in folder are .fen or list matches index.
                // A safer way is to rebuild the path from the selected text, but using the array is faster.
                const char *selectedPath = loadFilePaths.paths[loadFileActiveIndex];

                if (FileExists(selectedPath))
                {
                    // 2. Read FEN from file
                    char *loadedFen = LoadFileText(selectedPath);

                    if (loadedFen != NULL)
                    {
                        // 3. Reset & Load using the shared helper
                        LoadGameFromFEN(loadedFen);

                        UnloadFileText(loadedFen);

                        // Close Popup
                        showLoadFileDialog = false;
                        state.isInputLocked = false;
                    }
                }
            }
        }

        // Cancel Button for load
        if (GuiButton((Rectangle){winRect.x + winRect.width - 90, winRect.y + winRect.height - 40, 80, 30}, "Cancel"))
        {
            showLoadFileDialog = false;
            state.isInputLocked = false;
        }
    }
}
