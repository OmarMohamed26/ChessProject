/**
 * draw.c
 *
 * Responsibilities:
 * - Compute board layout and cell positions based on current window size.
 * - Render the chess board and pieces, including rank/file annotations.
 * - Manage interactive selection state, highlight borders, and last-move feedback.
 * - Load piece images from disk, convert to Texture2D and assign them to GameBoard cells.
 *
 * Public functions (exported in draw.h):
 * - void DrawBoard(int ColorTheme);
 *     Renders the board and all pieces. Should be called each frame inside
 *     BeginDrawing()/EndDrawing(). Computes layout for the current window size
 *     and calls display routines.
 *
 *  note that this function has been updated and this is no longer its signature
 * - void LoadPiece(int row, int col, PieceType type, Team team, int squareLength);
 *     Loads a piece image from assets/ and assigns the resulting Texture2D to
 *     GameBoard[row][col].piece.texture. If a texture already exists in that
 *     cell it is UnloadTexture()'d before assignment. Caller must ensure row/col
 *     are in [0,7]. `squareLength` controls image resize dimensions.
 *
 * - int ComputeSquareLength(void);
 *     Returns the computed size (in pixels) of a single board square using the
 *     current render width/height. Useful to compute positions/resources from
 *     main after InitWindow() or after a resize event.
 *
 * Notes / conventions:
 * - Filenames are generated as "assets/<piece><W|B>.png" (example: assets/kingW.png).
 * - TrimTrailingWhitespace removes trailing whitespace (useful if names come from
 *   user input). On Linux trailing spaces are significant in filenames so avoid them.
 * - LoadPiece will log warnings on failure (TraceLog). After LoadPiece returns,
 *   check GameBoard[row][col].piece.texture.id to confirm success (non-zero).
 * - This module uses static (file-local) helper functions. None of them are
 *   thread-safe; all operations are expected to be called from the main thread.
 * - Selection helpers (DecideDestination/SmartBorder utilities) store state between
 *   frames; treat them as single-threaded UI helpers.
 */

#include "draw.h"
#include "colors.h"
#include "main.h"
#include "move.h"
#include "raylib.h"
#include "settings.h"
#include "stack.h"
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Used in Make selected and last move borders */
typedef struct SmartBorder
{
    int row, col;
    Rectangle rect;
} SmartBorder;

// Local Prototypes
static int Min2(int num1, int num2);
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type, LoadPlace place);
// CHANGED: Added extraY parameter
static void InitializeCellsPos(int extraX, int extraY, int squareLength, float spaceText);
static size_t TrimTrailingWhitespace(char *string);
static void displayPieces(void);
static void DecideDestination(Vector2 topLeft);
static bool CompareCells(Cell *cell1, Cell *cell2);
static void swap(int *num1, int *num2);
static void SetCellBorder(SmartBorder *border, Cell *selectedPiece);
static void ResetCellBorder(SmartBorder *border);
static void ResizeCellBorder(SmartBorder *border);
static void ResetSelection();
static int Clamp(int num, int max);
static void HandlePromotionInput(void);
static void DrawPromotionMenu(void); // <--- ADD THIS PROTOTYPE
void DrawGameStatus(void);           // <--- ADD THIS PROTOTYPE

// This constant determines How much space is left for the text in terms of squareLength
#define SPACE_TEXT 0.75f

// NEW: Reserve space for 2 rows of squares at the top (Status Bar + Future Buttons)
#define TOP_SECTION_SQUARES 2.0f

// Local variables
bool IsSelectedPieceEmpty; // made this global because I need it in my highlight square function

Cell imaginaryCell = {.row = -1, .col = -1};

// The program depends on these values to say there shouldn't be any rect to be drawn so be careful
// SmartBorder selectedCellBorder = {.rect.x = -1, .rect.y = -1};
// SmartBorder lastMoveCellBorder = {.rect.x = -1, .rect.y = -1};

SmartBorder selectedCellBorder = {.rect.x = -1, .rect.y = -1};
SmartBorder lastMoveCellBorder = {.rect.x = -1, .rect.y = -1};

/**
 * DrawBoard
 *
 * Compute layout based on current render size and draw the board and pieces.
 *
 * Parameters:
 *  - ColorTheme: index into PALETTE (colors.h)
 *
 * Behavior:
 *  - Calls ComputeSquareLength() to obtain square size (in pixels).
 *  - Initializes cell positions (InitializeCellsPos) using the computed values.
 *  - Draws the 8x8 board using the chosen ColorPair.
 *  - Calls displayPieces() to render piece textures at computed positions.
 */
void DrawBoard(int ColorTheme, bool showFileRank)
{
    ColorPair theme = PALETTE[ColorTheme];
    float squareCount = BOARD_SIZE + SPACE_TEXT;
    int squareLength = ComputeSquareLength();

    // Horizontal Centering
    int extraX = (int)((float)GetRenderWidth() - (squareCount * (float)squareLength)) / 2;

    // NEW: Vertical Centering
    // Calculate how much vertical space the board + padding actually takes
    float verticalSquares = BOARD_SIZE + SPACE_TEXT + TOP_SECTION_SQUARES;
    int extraY = (int)((float)GetRenderHeight() - (verticalSquares * (float)squareLength)) / 2;

    // Safety clamp
    if (extraY < 0)
        extraY = 0;

    // Pass both offsets to the initialization function
    InitializeCellsPos(extraX, extraY, squareLength, SPACE_TEXT);

    // Draw the chess board (row = y, col = x)
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Color colr = ((row + col) & 1) ? theme.black : theme.white;
            DrawRectangleV(GameBoard[row][col].pos, (Vector2){(float)squareLength, (float)squareLength}, colr);
        }
    }

    // Font
    if (showFileRank)
    {
        //  compute once (matches InitializeCellsPos math)
        float boardLeft = (float)extraX + ((float)squareLength * SPACE_TEXT / 2);

        // Draw rank numbers (left) and file letters (bottom), centered in each square.
        int fontSize = (int)((float)squareLength / FONT_SQUARE_LENGTH_COEFFICIENT);
        if (fontSize < FONT_MIN)
        {
            fontSize = FONT_MIN;
        }
        if (fontSize > squareLength)
        {
            fontSize = squareLength;
        }

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            char rankText[2] = {(char)('8' - row), '\0'};
            int textWidth = MeasureText(rankText, fontSize);
            float textPosX = boardLeft - (float)textWidth - ((float)fontSize / FONT_GAP_COEFFICIENT); // small gap
            float textPosY = GameBoard[row][0].pos.y + ((float)(squareLength - fontSize) / 2);
            DrawText(rankText, (int)textPosX, (int)textPosY, fontSize, FONT_COLOR);
        }

        for (int col = 0; col < BOARD_SIZE; col++)
        {
            char fileText[2] = {(char)('a' + col), '\0'};
            int textWidth = MeasureText(fileText, fontSize);
            float textPosX = GameBoard[BOARD_SIZE - 1][col].pos.x + ((float)(squareLength - textWidth) / 2);

            // CHANGED: Calculate Y relative to the actual board position (bottom of last row)
            // This ensures text stays attached to the board even when we push the board down.
            float textPosY = GameBoard[BOARD_SIZE - 1][col].pos.y + (float)squareLength + ((float)fontSize / FONT_GAP_COEFFICIENT);

            DrawText(fileText, (int)textPosX, (int)textPosY, fontSize, FONT_COLOR);
        }
    }

    DecideDestination(GameBoard[0][0].pos);

    if (IsWindowResized())
    {
        ResizeCellBorder(&selectedCellBorder);
        ResizeCellBorder(&lastMoveCellBorder);
    }

    int borderThickness = (int)round(squareLength / (double)CELL_BORDER_THICKNESS_COEFFICIENT);

    if (selectedCellBorder.rect.x != -1 && selectedCellBorder.rect.y != -1)
    {
        DrawRectangleLinesEx(selectedCellBorder.rect, (float)borderThickness, SELECTED_BORDER_COLOR);
    }

    if (lastMoveCellBorder.rect.x != -1 && lastMoveCellBorder.rect.y != -1)
    {
        DrawRectangleLinesEx(lastMoveCellBorder.rect, (float)borderThickness, LAST_MOVE_BORDER_COLOR);
    }

    displayPieces();

    // <--- ADD THIS BLOCK AT THE END OF DrawBoard
    if (state.isPromoting)
    {
        DrawPromotionMenu();
    }

    DrawGameStatus();
}

/**
 * LoadPiece
 *
 * Public helper to load a piece texture and store it in GameBoard[row][col].
 * It selects the filename by piece type and team and delegates to LoadHelper.
 *
 * Parameters:
 *  - row, col : board coordinates (0..7)
 *  - type     : PieceType enum
 *  - team     : TEAM_WHITE or TEAM_BLACK
 *  - LoadPlace: this allows us to use the function for multiple purposes (it takes an enum)
 *
 * Safety:
 *  - Performs bounds check on row/col.
 *  - LoadHelper handles logging and texture assignment.
 */
void LoadPiece(int row, int col, PieceType type, Team team, LoadPlace place)
{
    if (place == GAME_BOARD)
    {
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        {
            return;
        }
    }
    else if (place == DEAD_WHITE_PIECES || place == DEAD_BLACK_PIECES)
    {
        if (row < 0 || row >= 2 * BOARD_SIZE)
        {
            return;
        }
    }

    char pieceName[MAX_PIECE_NAME_BUFFER_SIZE + 1];

    switch (type)
    {
    case PIECE_PAWN:
        LoadHelper(pieceName, sizeof pieceName, "pawn", team, row, col, type, place);
        break;
    case PIECE_KNIGHT:
        LoadHelper(pieceName, sizeof pieceName, "knight", team, row, col, type, place);
        break;
    case PIECE_BISHOP:
        LoadHelper(pieceName, sizeof pieceName, "bishop", team, row, col, type, place);
        break;
    case PIECE_ROOK:
        LoadHelper(pieceName, sizeof pieceName, "rook", team, row, col, type, place);
        break;
    case PIECE_QUEEN:
        LoadHelper(pieceName, sizeof pieceName, "queen", team, row, col, type, place);
        break;
    case PIECE_KING:
        LoadHelper(pieceName, sizeof pieceName, "king", team, row, col, type, place);
        break;
    default:
        break;
    }
}

/**
 * LoadHelper (static)
 *
 * Build filename, load image, resize, create texture, and assign to GameBoard cell.
 * This function logs failures and ensures any existing texture in the target cell
 * is unloaded before assigning the new one.
 *
 * Parameters:
 *  - pieceNameBuffer, bufferSize: output buffer for the generated filename
 *  - pieceName: base name (e.g. "king", "pawn")
 *  - team: TEAM_WHITE/TEAM_BLACK
 *  - squareLength: resize dimension
 *  - row/col: target cell coordinates
 *  - type: PieceType to set on the cell
 */
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type, LoadPlace place)
{
    /*This function loads the texture for any given piece correctly and handles errors
    and puts a new texture if one already exists at this cell*/

    int length = snprintf(pieceNameBuffer, (size_t)bufferSize, "assets/pieces/%s%c.png", pieceName, (team == TEAM_WHITE) ? 'W' : 'B');
    if (length < 0)
    {
        return;
    }
    if (length >= bufferSize)
    {
        TraceLog(LOG_WARNING, "Filename was truncated: %s", pieceNameBuffer);
    }

    TrimTrailingWhitespace(pieceNameBuffer);

    Texture2D texture = LoadTexture(pieceNameBuffer);

    if (texture.id == 0 || texture.width == 0 || texture.height == 0)
    {
        TraceLog(LOG_WARNING, "Failed to load texture: %s", pieceNameBuffer);
        return;
    }

    if (texture.width != texture.height)
    {
        TraceLog(LOG_WARNING, "Invalid texture Error, texture's width must equal it's height");
        return;
    }

    if (place == GAME_BOARD)
    {
        // unload previous texture if present
        if (GameBoard[row][col].piece.texture.id != 0)
        {
            UnloadTexture(GameBoard[row][col].piece.texture);
        }

        // Add the piece to the GameBoard
        GameBoard[row][col].piece.texture = texture;
        GameBoard[row][col].piece.type = type;
        GameBoard[row][col].piece.team = team;
    }
    else if (place == DEAD_WHITE_PIECES)
    {
        printf("Loaded a piece in dead white.\n");
        DeadWhitePieces[row].piece.texture = texture;
        DeadWhitePieces[row].piece.type = type;
        DeadWhitePieces[row].piece.team = team;
    }
    else if (place == DEAD_BLACK_PIECES)
    {
        DeadBlackPieces[row].piece.texture = texture;
        DeadBlackPieces[row].piece.type = type;
        DeadBlackPieces[row].piece.team = team;
    }
}

/**
 * displayPieces (static)
 *
 * Draw all loaded piece textures stored in GameBoard. If a cell's piece type
 * is PIECE_NONE it is skipped. Uses DrawTextureEx to place the texture at the
 * precomputed GameBoard[row][col].pos position.
 *
 */
static void displayPieces(void) // and DeadPieces
{
    // Draw pieces using same row/col ordering
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            if (GameBoard[row][col].piece.type != PIECE_NONE)
            {
                DrawTextureEx(GameBoard[row][col].piece.texture, GameBoard[row][col].pos, 0, (float)ComputeSquareLength() / (float)GameBoard[row][col].piece.texture.width, WHITE);
            }
        }
    }

    // Display Dead White Pieces
    for (int row = 0; row < deadWhiteCounter; row++)
    {
        if (DeadWhitePieces[row].piece.type != PIECE_NONE)
        {
            DrawTextureEx(DeadWhitePieces[row].piece.texture, DeadWhitePieces[row].pos, 0, (float)ComputeSquareLength() / (4 * (float)DeadWhitePieces[row].piece.texture.width), WHITE);
        }
    }

    // Display Dead Black Pieces
    for (int row = 0; row < deadBlackCounter; row++)
    {
        if (DeadBlackPieces[row].piece.type != PIECE_NONE)
        {
            DrawTextureEx(DeadBlackPieces[row].piece.texture, DeadBlackPieces[row].pos, 0, (float)ComputeSquareLength() / (4 * (float)DeadBlackPieces[row].piece.texture.width), WHITE);
        }
    }
}

/**
 * InitializeCellsPos (static)
 *
 * Compute and store the top-left position (Vector2) for each board cell in GameBoard.
 *
 * Parameters:
 *  - extraX: horizontal offset to center the board
 *  - extraY: vertical offset to center the board
 *  - squareLength: size of each square in pixels
 *  - spaceText: fractional extra space used when computing board layout (SPACE_TEXT)
 *
 * Calling pattern:
 *  - Compute squareLength and extra in DrawBoard (or main), then call this to set positions.
 */
static void InitializeCellsPos(int extraX, int extraY, int squareLength, float spaceText) // It also Initializes dead cells
{
    // NEW: Push the board down by TOP_SECTION_SQUARES PLUS the vertical centering offset
    float topOffset = (float)extraY + ((float)squareLength * TOP_SECTION_SQUARES);

    // row = y, col = x
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].pos = (Vector2){
                (float)extraX + ((float)squareLength * spaceText / 2) + ((float)col * (float)squareLength), // x = col
                topOffset + ((float)row * (float)squareLength) + ((float)squareLength * spaceText / 2)      // y = row + offset
            };
        }
    }

    // Initialize DeadWhite Cells
    Vector2 topLeft = GameBoard[0][0].pos;
    for (int row = 0; row < 2 * BOARD_SIZE; row++)
    {
        DeadWhitePieces[row].pos = (Vector2){topLeft.x + ((float)row * (float)squareLength / 4), topLeft.y - (float)((float)squareLength / 4)};
    }

    Vector2 topMiddle = GameBoard[0][4].pos;
    for (int row = 0; row < 2 * BOARD_SIZE; row++)
    {
        DeadBlackPieces[row].pos = (Vector2){topMiddle.x + ((float)row * (float)squareLength / 4), topMiddle.y - (float)((float)squareLength / 4)};
    }
}

/**
 * TrimTrailingWhitespace (static)
 *
 * Remove trailing whitespace from a NUL-terminated C string in-place and return the new length.
 * Whitespace is tested with isspace((unsigned char)c).
 *
 * Returns:
 *  - new length (size_t) after trimming.
 */
static size_t TrimTrailingWhitespace(char *string)
{
    // Check to see if it's an empty sting
    if (!string)
    {
        return 0;
    }
    size_t len = strlen(string);
    while (len > 0 && isspace((unsigned char)string[len - 1]))
    {
        len--;
    }
    string[len] = '\0';
    return len;
}

/**
 * Min2 (static)
 *
 * Return the smaller of two ints.
 */
static int Min2(int num1, int num2)
{
    return num1 < num2 ? num1 : num2;
}

static void ResetSelection()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            GameBoard[i][j].selected = false;
        }
    }
}

/**
 * ComputeSquareLength
 *
 * Public helper to compute a reasonable square size given the current render
 * width/height and the SPACE_TEXT constant. This is useful to compute positions
 * or to pass to LoadPiece from main after InitWindow() so that resource sizing
 * and layout logic agree.
 */
int ComputeSquareLength()
{
    float horizontalSquares = BOARD_SIZE + SPACE_TEXT;

    // NEW: Account for vertical space (Board + Text + Top Padding)
    float verticalSquares = BOARD_SIZE + SPACE_TEXT + TOP_SECTION_SQUARES;

    int sizeByWidth = (int)((float)GetRenderWidth() / horizontalSquares);
    int sizeByHeight = (int)((float)GetRenderHeight() / verticalSquares);

    // CHANGED: Return the smaller size to ensure it fits in both dimensions
    return Min2(sizeByWidth, sizeByHeight);
}

/**
 * InitializeBoard
 *
 * Reset every GameBoard cell to its default coordinates and empty state.
 *
 * Behavior:
 * - Writes the row/col indices into each Cell (used later for lookups).
 * - Calls SetEmptyCell so textures, pieces, and metadata are cleared.
 *
 * Usage:
 * - Call once during startup, or before loading a fresh position.
 * - Textures are not unloaded here; invoke UnloadBoard when replacing assets.
 */
void InitializeBoard(void)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            GameBoard[i][j].row = i;
            GameBoard[i][j].col = j;
            SetEmptyCell(&GameBoard[i][j]);
        }
    }
}

void InitializeDeadPieces(void)
{
    for (int row = 0; row < 2 * BOARD_SIZE; row++)
    {
        DeadWhitePieces[row].piece.type = PIECE_NONE;
        DeadBlackPieces[row].piece.type = PIECE_NONE;
    }
}

/**
 * UnloadBoard
 *
 * Release all piece textures stored in GameBoard and reset cells to empty.
 *
 * Behavior:
 * - Iterates every cell; if a piece texture is present it is UnloadTexture()'d.
 * - Calls SetEmptyCell afterwards to mark PIECE_NONE and clear metadata.
 *
 * Usage:
 * - Invoke when shutting down, reloading themes, or replacing the full board.
 * - Safe to call multiple times; SetEmptyCell handles already-empty cells.
 */
void UnloadBoard(void)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (GameBoard[i][j].piece.type != PIECE_NONE)
            {
                UnloadTexture(GameBoard[i][j].piece.texture);
            }

            SetEmptyCell(&GameBoard[i][j]);
        }
    }
}

void UnloadDeadPieces(void)
{
    for (int row = 0; row < 2 * BOARD_SIZE; row++)
    {
        if (DeadWhitePieces[row].piece.type != PIECE_NONE)
        {
            UnloadTexture(DeadWhitePieces[row].piece.texture);
        }
        else
        {
            break;
        }
    }

    for (int row = 0; row < 2 * BOARD_SIZE; row++)
    {
        if (DeadBlackPieces[row].piece.type != PIECE_NONE)
        {
            UnloadTexture(DeadBlackPieces[row].piece.texture);
        }
        else
        {
            break;
        }
    }
}

void HighlightSquare(int row, int col, int ColorTheme)
{ // This now fixes the select bug
    ColorPair theme = PALETTE[ColorTheme];
    int squareLength = ComputeSquareLength();
    Color colr = ((row + col) & 1) ? theme.black : theme.white;
    colr.r = Clamp(colr.r + HIGHLIGHT_COLOR_AMOUNT, MAX_VALID_COLOR);
    colr.g = Clamp(colr.g + HIGHLIGHT_COLOR_AMOUNT, MAX_VALID_COLOR);
    colr.b = Clamp(colr.b + HIGHLIGHT_COLOR_AMOUNT, MAX_VALID_COLOR);
    DrawRectangleV(GameBoard[row][col].pos, (Vector2){(float)squareLength, (float)squareLength}, colr);
    if (GameBoard[row][col].piece.texture.id != 0)
    {
        DrawTextureEx(GameBoard[row][col].piece.texture, GameBoard[row][col].pos, 0, (float)ComputeSquareLength() / (float)GameBoard[row][col].piece.texture.width, WHITE);
    }
}

static int Clamp(int num, int max)
{
    return (num <= max) ? num : max;
}

/**
 * HighlightHover
 *
 * Purpose:
 *  - Provide a per-frame hover feedback for the board square under the mouse.
 *    When appropriate this will visually highlight the square and set a
 *    pointing-hand mouse cursor so the UI indicates a selectable piece.
 *
 * Behavior:
 *  - Maps the current mouse position into board coordinates (row/col) using
 *    GameBoard[0][0].pos as the top-left origin and the computed square size.
 *  - If the mouse is over the board and the square contains a piece, the
 *    function will highlight that square only when no piece is currently
 *    selected (IsSelectedPieceEmpty == true) and the piece belongs to the
 *    side whose turn it is (global Turn).
 *  - When a highlight is applied the mouse cursor is changed to MOUSE_CURSOR_POINTING_HAND.
 *    Otherwise the cursor is kept as MOUSE_CURSOR_ARROW.
 *
 * Notes / Caveats:
 *  - This function relies on InitializeCellsPos(...) having been called so
 *    GameBoard[*][*].pos values reflect the current layout. Call ComputeSquareLength()
 *    / InitializeCellsPos after any window resize before using this.
 *  - It uses globals row and col to expose the computed row/col index of the hovered
 *    cell for other code that may inspect it (they are assigned when the mouse
 *    is inside the board).
 *  - It only highlights owned pieces when it's that team's turn.
 *
 * Parameters:
 *  - ColorTheme: index into PALETTE used to pick the board colors when drawing
 *                the temporary hover highlight.
 */
void HighlightHover(int ColorTheme)
{
    int Sql = ComputeSquareLength();
    int X_Pos = (GetMouseX());
    int Y_Pos = (GetMouseY());
    float ratioX;
    float ratioY;
    float Max_Board_X = (GameBoard[0][BOARD_SIZE - 1].pos.x + (float)Sql);
    float Max_Board_Y = (GameBoard[BOARD_SIZE - 1][0].pos.y + (float)Sql);

    // Added local declarations here
    int col = 0;
    int row = 0;

    // Mouse coordinate checking
    if ((float)X_Pos >= GameBoard[0][0].pos.x && (float)X_Pos <= Max_Board_X &&
        (float)Y_Pos >= GameBoard[0][0].pos.y && (float)Y_Pos <= Max_Board_Y)
    {
        ratioX = (((float)X_Pos - GameBoard[0][0].pos.x) * BOARD_SIZE) / (Max_Board_X - GameBoard[0][0].pos.x);
        col = (int)ratioX;
        ratioY = (((float)Y_Pos - GameBoard[0][0].pos.y) * BOARD_SIZE) / (Max_Board_Y - GameBoard[0][0].pos.y);
        row = (int)ratioY;
        if (GameBoard[row][col].piece.type != PIECE_NONE)
        {
            if (IsSelectedPieceEmpty) // Makes hover highlight effect only when no piece is selected
            {
                if (GameBoard[row][col].piece.team == Turn)
                {
                    HighlightSquare(row, col, ColorTheme);
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                }

            } // Nice effect added where hover only works when It's the team's turn
        }
        else
        {
            SetMouseCursor(MOUSE_CURSOR_ARROW); // this fixes twitching of cursor
        }
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

/**
 * DecideDestination (static)
 *
 * Handle click-to-select and click-to-move interactions on the board.
 *
 * Description:
 *  - Implements simple two-click piece movement: first left-click selects a
 *    non-empty cell (source) and highlights it; the second left-click is an
 *    attempt to move the selected piece to the clicked cell (destination).
 *  - If the destination is out of bounds the selection is cleared.
 *  - On a valid (or invalid) destination MovePiece(...) is called and last-move highlighting
 *    is updated.
 *
 * Parameters:
 *  - topLeft: absolute top-left pixel of the (0,0) board cell; used to map
 *             window mouse coordinates into board row/column indices.
 *
 * Side effects:
 *  - Updates selectedCellBorder and lastMoveCellBorder SmartBorder state.
 *  - Calls MovePiece(CellX, CellY, NewCellX, NewCellY) when a move is made.
 *  - Logs selection/move events with TraceLog.
 *
 * Notes:
 *  - This function relies on ComputeSquareLength() and GameBoard positions
 *    previously initialized by InitializeCellsPos(...).
 *  - It is intended to be called once per frame from DrawBoard().
 */
static void DecideDestination(Vector2 topLeft)
{
    // NEW: Intercept input if promoting
    if (state.isPromoting)
    {
        HandlePromotionInput();
        return; // Stop here, do not run normal logic
    }

    bool TurnValidation = false;

    ResetSelection(); // this is to set all the selection values to false

    static int CellX = -1;
    static int CellY = -1;

    // It's initially equal to imaginaryCell but I can't write it directly
    static Cell selectedPiece = {.row = -1, .col = -1};

    IsSelectedPieceEmpty = CompareCells(&selectedPiece, &imaginaryCell);

    int SquareLength = ComputeSquareLength();

    // Pick a piece while You don't hold one
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsSelectedPieceEmpty)
    {
        CellX = (GetMouseX() - (int)topLeft.x) / SquareLength;
        CellY = (GetMouseY() - (int)topLeft.y) / SquareLength;

        swap(&CellX, &CellY);

        if (CellX < 0 || CellX > (BOARD_SIZE - 1) || CellY < 0 || CellY > (BOARD_SIZE - 1))
        {
            return;
        }
        if (Turn == GameBoard[CellX][CellY].piece.team)
        {
            TurnValidation = true;
        }
        if (GameBoard[CellX][CellY].piece.type != PIECE_NONE && TurnValidation) // We try to pick a piece in our turn
        {
            selectedPiece = GameBoard[CellX][CellY];
            selectedPiece.selected = true;
            SetCellBorder(&selectedCellBorder, &selectedPiece);
            TraceLog(LOG_DEBUG, "Selected A new Piece: %d %d", CellX, CellY);
            PrimaryValidation(selectedPiece.piece.type, CellX, CellY, true); // Renamed Validateanddevalidate to just validate
            FinalValidation(CellX, CellY, selectedPiece.selected);
        }
    }
    HighlightValidMoves(selectedPiece.selected);
    // Move the piece if you hold one
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsSelectedPieceEmpty)
    {
        int NewCellX = (GetMouseX() - (int)topLeft.x) / SquareLength;
        int NewCellY = (GetMouseY() - (int)topLeft.y) / SquareLength;

        swap(&NewCellX, &NewCellY);

        if (NewCellX < 0 || NewCellX > (BOARD_SIZE - 1) || NewCellY < 0 || NewCellY > (BOARD_SIZE - 1))
        {
            selectedPiece.selected = false;
            ResetCellBorder(&selectedCellBorder);
            ResetValidation();
            ResetPrimaryValidation(); // replaced old big function with just reset validation
            selectedPiece = imaginaryCell;
            TraceLog(LOG_DEBUG, "Unselected the piece because you tried to move it to an invalid pos");
            return;
        }

        // I add this part to unselect a piece if you click on an invalid position
        if (!GameBoard[NewCellX][NewCellY].isvalid)
        {
            selectedPiece.selected = false;
            ResetCellBorder(&selectedCellBorder);
            ResetValidation();
            ResetPrimaryValidation(); // replaced old big function with just reset validation
            selectedPiece = imaginaryCell;
            TraceLog(LOG_DEBUG, "Unselected the piece because you tried to move it to an invalid pos");
            return;
        }

        if (NewCellX == CellX && NewCellY == CellY)
        {
            selectedPiece.selected = false;
            ResetCellBorder(&selectedCellBorder);
            ResetValidation();
            ResetPrimaryValidation();
            selectedPiece = imaginaryCell;
            return;
        }

        if (GameBoard[NewCellX][NewCellY].isvalid)
        {
            MovePiece(CellX, CellY, NewCellX, NewCellY);
            SetCellBorder(&lastMoveCellBorder, &GameBoard[NewCellX][NewCellY]);
            TraceLog(LOG_DEBUG, "%d %d %d %d", CellX, CellY, NewCellX, NewCellY);
            TraceLog(LOG_DEBUG, "Moved the selected piece to the new pos: %d %d", NewCellX, NewCellY);
            selectedPiece.selected = false;
            ResetValidation();
            ResetPrimaryValidation();
            selectedPiece = imaginaryCell;
        }
    }
}

/**
 * CompareCells (static)
 *
 * Determine whether two cells refer to the same board coordinates.
 *
 * Parameters:
 *  - cell1, cell2: pointers to cells to compare.
 *
 * Returns:
 *  - true if both row and column match, false otherwise.
 */
static bool CompareCells(Cell *cell1, Cell *cell2)
{
    // This is not a full comparison but it's enough for our usage
    if (cell1->row == cell2->row && cell1->col == cell2->col)
    {
        return true;
    }
    return false;
}

/**
 * swap (static)
 *
 * Exchange two integer values in-place.
 *
 * Parameters:
 *  - num1, num2: pointers to the integers to swap.
 */
static void swap(int *num1, int *num2)
{
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

/**
 * SetCellBorder (static)
 *
 * Configure a SmartBorder to track the given cell's position and size.
 * Computes the border rectangle using the current square length.
 *
 * Parameters:
 *  - border: SmartBorder to configure.
 *  - selectedPiece: cell whose position should be highlighted.
 */
static void SetCellBorder(SmartBorder *border, Cell *selectedPiece)
{
    border->rect.width = border->rect.height = (float)ComputeSquareLength();
    border->rect.x = selectedPiece->pos.x;
    border->rect.y = selectedPiece->pos.y;
    border->row = selectedPiece->row;
    border->col = selectedPiece->col;
}

/**
 * ResetCellBorder (static)
 *
 * Disable a SmartBorder by setting its rectangle dimensions to -1.
 *
 * Parameters:
 *  - border: SmartBorder to reset.
 */
static void ResetCellBorder(SmartBorder *border)
{
    border->rect.width = border->rect.height = -1;
}

/**
 * ResizeCellBorder (static)
 *
 * Recompute the SmartBorder rectangle size and position after a resize event.
 * Utilizes the stored row/col indices to locate the updated cell position.
 *
 * Parameters:
 *  - border: SmartBorder to update.
 */
static void ResizeCellBorder(SmartBorder *border)
{
    border->rect.width = border->rect.height = (float)ComputeSquareLength();
    if (border->rect.x != -1 && border->rect.y != -1)
    {
        border->rect.x = GameBoard[border->row][border->col].pos.x;
        border->rect.y = GameBoard[border->row][border->col].pos.y;
    }
}

/**
 * HighlightValidMoves
 *
 * Purpose:
 *  - When a piece is currently selected, render per-frame visual markers on
 *    every board cell that has its `isvalid` flag set. These markers indicate
 *    legal destination squares for the selected piece.
 *
 * Behavior:
 *  - If `selected` is false the function does nothing.
 *  - Computes sizes from ComputeSquareLength() so markers scale with the board.
 *  - Iterates the global GameBoard and for each cell with isvalid==true draws
 *    a small filled circle centered if the cell is empty in that square using VALID_MOVE_COLOR
 *    otherwise it draws a hollow circle.
 *
 * Parameters:
 *  - selected : boolean indicating whether a piece is selected (only then draw).
 *
 * Notes / Side-effects:
 *  - Uses the globals `row` and `col` as loop indices.
 *  - Relies on GameBoard[*][*].pos being initialized (InitializeCellsPos called).
 *  - Rendering occurs immediately (per-frame); call from DrawBoard() during drawing.
 */
void HighlightValidMoves(bool selected)
{
    if (selected)
    {
        int halfSquareLength = ComputeSquareLength() / 2;
        int validMoveCircleRadius = (int)round(halfSquareLength / (double)VALID_MOVE_CIRCLE_SQUARE_COEFFICIENT);
        int innerRingRadius = (int)((float)halfSquareLength * (INNER_VALID_MOVE_RADIUS / (float)FULL_VALID_MOVE_RADIUS));
        int outerRingRadius = (int)((float)halfSquareLength * (OUTER_VALID_MOVE_RADIUS / (float)FULL_VALID_MOVE_RADIUS));

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                Cell thisCell = GameBoard[row][col];
                if (thisCell.isvalid)
                {
                    Vector2 centerPos = thisCell.pos;
                    centerPos.x += (float)halfSquareLength;
                    centerPos.y += (float)halfSquareLength;

                    if (thisCell.piece.type == PIECE_NONE)
                    {
                        DrawCircleV(centerPos, (float)validMoveCircleRadius, VALID_MOVE_COLOR);
                    }
                    else
                    {
                        DrawRing(centerPos, (float)innerRingRadius, (float)outerRingRadius, 0, FULL_CIRCLE_ANGLE, 25 /*this is the resolution just leave it as is*/, VALID_MOVE_COLOR);
                        // DrawRingLines(centerPos, innerRingRadius, halfSquareLength, 0, 360, 25, VALID_MOVE_COLOR);
                    }
                }
            }
        }
    }
}

static void DrawPromotionMenu(void)
{
    if (!state.isPromoting)
    {
        return;
    }

    int row = state.promotionRow;
    int col = state.promotionCol;
    int squareSize = ComputeSquareLength();
    int startX = GameBoard[row][col].pos.x;
    int startY = GameBoard[row][col].pos.y;

    // Direction: White (row 0) draws down, Black (row 7) draws up
    int direction = (row == 0) ? 1 : -1;

    // Draw Background
    int menuHeight = squareSize * 4;
    int menuY = (direction == 1) ? startY : (startY - (squareSize * 3));

    DrawRectangle(startX, menuY, squareSize, menuHeight, Fade(LIGHTGRAY, 0.9f));
    DrawRectangleLines(startX, menuY, squareSize, menuHeight, DARKGRAY);

    // Draw Options (Placeholder Text for now, replace with Textures later)
    const char *names[] = {"Q", "R", "B", "N"};

    for (int i = 0; i < 4; i++)
    {
        int yPos = startY + (direction * squareSize * i);
        Rectangle btn = {startX, yPos, squareSize, squareSize};

        // Hover effect
        if (CheckCollisionPointRec(GetMousePosition(), btn))
        {
            DrawRectangleRec(btn, Fade(WHITE, 0.5f));
        }

        // Centered Text
        int fontSize = squareSize / 2;
        int textWidth = MeasureText(names[i], fontSize);
        DrawText(names[i], startX + (squareSize - textWidth) / 2, yPos + (squareSize - fontSize) / 2, fontSize, BLACK);
    }
}

static void HandlePromotionInput(void)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return;

    int row = state.promotionRow;
    int col = state.promotionCol;
    int squareLength = ComputeSquareLength();
    int startX = GameBoard[row][col].pos.x;
    int startY = GameBoard[row][col].pos.y;
    int direction = (row == 0) ? 1 : -1;

    Rectangle queenRect = {startX, startY, (float)squareLength, (float)squareLength};
    Rectangle rookRect = {startX, startY + (direction * squareLength * 1), (float)squareLength, (float)squareLength};
    Rectangle bishopRect = {startX, startY + (direction * squareLength * 2), (float)squareLength, (float)squareLength};
    Rectangle knightRect = {startX, startY + (direction * squareLength * 3), (float)squareLength, (float)squareLength};

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, queenRect))
        PromotePawn(PIECE_QUEEN);
    else if (CheckCollisionPointRec(mouse, rookRect))
        PromotePawn(PIECE_ROOK);
    else if (CheckCollisionPointRec(mouse, bishopRect))
        PromotePawn(PIECE_BISHOP);
    else if (CheckCollisionPointRec(mouse, knightRect))
        PromotePawn(PIECE_KNIGHT);
}

void UpdateLastMoveHighlight(int row, int col)
{
    if (row < 0 || col < 0)
    {
        ResetCellBorder(&lastMoveCellBorder);
    }
    else
    {
        SetCellBorder(&lastMoveCellBorder, &GameBoard[row][col]);
    }
}

void DrawDebugInfo(void)
{
    int x = 10;
    int y = 10;
    int fontSize = DEBUG_MENU_FONT_SIZE;
    int step = DEBUG_MENU_FONT_SIZE + SPACE_BETWEEN_DEBUG_LINES;
    Color textColor = DEBUG_TEXT_COLOR;

    // Background
    DrawRectangle(0, 0, 300, 470, Fade(BLACK, 0.8f));

    DrawFPS(x, y);
    y += step;

    DrawText("--- GAME STATE ---", x, y, fontSize, GREEN);
    y += step;

    DrawText(TextFormat("Turn: %s", state.turn == TEAM_WHITE ? "WHITE" : "BLACK"), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("Full Moves: %d", state.fullMoveNumber), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("Half Move Clock: %d", state.halfMoveClock), x, y, fontSize, textColor);
    y += step;

    // En Passant
    unsigned char epTarget[3] = "-";
    if (state.enPassantCol != -1)
    {
        epTarget[0] = 'a' + state.enPassantCol;
        epTarget[1] = (state.turn == TEAM_WHITE) ? '6' : '3'; // If it's White's turn, target is rank 6, else 3
        epTarget[2] = '\0';
    }
    DrawText(TextFormat("En Passant Target: %s", epTarget), x, y, fontSize, textColor);
    y += step;

    // Castling Rights
    char castling[5] = "-";
    if (state.whiteKingSide)
    {
        castling[0] = 'K';
    }
    if (state.whiteQueenSide)
    {
        castling[1] = 'Q';
    }
    if (state.blackKingSide)
    {
        castling[2] = 'k';
    }
    if (state.blackQueenSide)
    {
        castling[3] = 'q';
    }
    DrawText(TextFormat("Castling: %s", castling), x, y, fontSize, textColor);
    y += step;

    y += SPACE_BETWEEN_DEBUG_SECTIONS; // Spacer
    DrawText("--- FLAGS ---", x, y, fontSize, GREEN);
    y += step;

    DrawText(TextFormat("White Checked: %s", state.whitePlayer.Checked ? "YES" : "NO"), x, y, fontSize, state.whitePlayer.Checked ? RED : GRAY);
    y += step;

    DrawText(TextFormat("Black Checked: %s", state.blackPlayer.Checked ? "YES" : "NO"), x, y, fontSize, state.blackPlayer.Checked ? RED : GRAY);
    y += step;

    DrawText(TextFormat("Checkmate: %s", state.isCheckmate ? "YES" : "NO"), x, y, fontSize, state.isCheckmate ? RED : textColor);
    y += step;

    DrawText(TextFormat("Stalemate: %s", state.isStalemate ? "YES" : "NO"), x, y, fontSize, state.isStalemate ? ORANGE : textColor);
    y += step;

    DrawText(TextFormat("3-Fold Rep: %s", state.isRepeated3times ? "YES" : "NO"), x, y, fontSize, state.isRepeated3times ? ORANGE : textColor);
    y += step;

    DrawText(TextFormat("Insufficient. Mat: %s", state.isInsufficientMaterial ? "YES" : "NO"), x, y, fontSize, state.isInsufficientMaterial ? ORANGE : textColor);
    y += step;

    y += SPACE_BETWEEN_DEBUG_SECTIONS; // Spacer
    DrawText("--- MEMORY ---", x, y, fontSize, GREEN);
    y += step;

    DrawText(TextFormat("Undo Stack: %zu", state.undoStack->size), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("Redo Stack: %zu", state.redoStack->size), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("History (DHA): %zu", state.DHA->size), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("Dead White: %d", deadWhiteCounter), x, y, fontSize, textColor);
    y += step;

    DrawText(TextFormat("Dead Black: %d", deadBlackCounter), x, y, fontSize, textColor);
}

void DrawGameStatus(void)
{
    const char *message = NULL;
    Color bgColor = BLANK;
    Color textColor = STATUS_TEXT_COLOR;

    // 1. Determine the message and color priority
    if (state.isCheckmate)
    {
        message = "CHECKMATE";
        bgColor = RED;
    }
    else if (state.isStalemate)
    {
        message = "STALEMATE";
        bgColor = DARKGRAY;
    }
    else if (state.isRepeated3times)
    {
        message = "DRAW (REPETITION)";
        bgColor = BLUE;
    }
    else if (state.isInsufficientMaterial)
    {
        message = "DRAW (INSUFFICIENT MATERIAL)";
        bgColor = BLUE;
    }
    else if (state.halfMoveClock >= 100)
    {
        message = "DRAW (50 MOVES)";
        bgColor = BLUE;
    }
    else if (state.whitePlayer.Checked)
    {
        message = "WHITE IS IN CHECK";
        bgColor = ORANGE;
        textColor = BLACK;
    }
    else if (state.blackPlayer.Checked)
    {
        message = "BLACK IS IN CHECK";
        bgColor = ORANGE;
        textColor = BLACK;
    }

    // 2. Draw the UI if there is a message
    if (message != NULL)
    {
        int screenWidth = GetRenderWidth();
        int fontSize = STATUS_MENU_FONT_SIZE;
        int padding = STATUS_MENU_PADDING;
        int textWidth = MeasureText(message, fontSize);

        int rectWidth = textWidth + (padding * 2);
        int rectHeight = fontSize + (padding * 2);
        int rectX = (screenWidth - rectWidth) / 2;

        // CHANGED: Position the status bar in the "second row" of the top section
        // Row 1 (Top) = Future Buttons
        // Row 2 (Bottom) = Status Bar
        int squareLength = ComputeSquareLength();

        // Calculate vertical centering offset (same as DrawBoard)
        float verticalSquares = BOARD_SIZE + SPACE_TEXT + TOP_SECTION_SQUARES;
        int extraY = (int)((float)GetRenderHeight() - (verticalSquares * (float)squareLength)) / 2;
        if (extraY < 0)
            extraY = 0;

        // Place it 1 square down from the top of the board area
        int rectY = extraY + squareLength + (squareLength - rectHeight) / 2;

        // Draw Shadow (cool effect)
        DrawRectangle(rectX + 4, rectY + 4, rectWidth, rectHeight, Fade(BLACK, 0.3f));

        // Draw Background
        DrawRectangle(rectX, rectY, rectWidth, rectHeight, Fade(bgColor, 0.9f));

        // Draw Border
        DrawRectangleLinesEx((Rectangle){(float)rectX, (float)rectY, (float)rectWidth, (float)rectHeight}, 4.0f, Fade(WHITE, 0.5f));

        // Draw Text
        DrawText(message, rectX + padding, rectY + padding, fontSize, textColor);
    }
}
