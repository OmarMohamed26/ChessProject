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
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// access the GameBoard from the main.c file
extern Cell GameBoard[BOARD_SIZE][BOARD_SIZE];

// Local Prototypes
static int Min2(int num1, int num2);
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type);
static void InitializeCellsPos(int extra, int squareLength, float spaceText);
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

// This constant determines How much space is left for the text in terms of squareLength
#define SPACE_TEXT 0.75f

// Local variables
int row, col;
int pointer;
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
    int extra = (int)((float)GetRenderWidth() - (squareCount * (float)squareLength)) / 2;

    InitializeCellsPos(extra, squareLength, SPACE_TEXT);

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
        float boardLeft = (float)extra + ((float)squareLength * SPACE_TEXT / 2);
        float boardTop = (float)squareLength * SPACE_TEXT / 2;

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
            float textPosY = (boardTop + (BOARD_SIZE * (float)squareLength) + ((float)fontSize / FONT_GAP_COEFFICIENT)); // below board
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
 *  - squareLength : desired texture dimension (square)
 *
 * Safety:
 *  - Performs bounds check on row/col.
 *  - LoadHelper handles logging and texture assignment.
 */
void LoadPiece(int row, int col, PieceType type, Team team)
{
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
    {
        return;
    }

    char pieceName[MAX_PIECE_NAME_BUFFER_SIZE + 1];

    switch (type)
    {
    case PIECE_PAWN:
        LoadHelper(pieceName, sizeof pieceName, "pawn", team, row, col, type);
        break;
    case PIECE_KNIGHT:
        LoadHelper(pieceName, sizeof pieceName, "knight", team, row, col, type);
        break;
    case PIECE_BISHOP:
        LoadHelper(pieceName, sizeof pieceName, "bishop", team, row, col, type);
        break;
    case PIECE_ROOK:
        LoadHelper(pieceName, sizeof pieceName, "rook", team, row, col, type);
        break;
    case PIECE_QUEEN:
        LoadHelper(pieceName, sizeof pieceName, "queen", team, row, col, type);
        break;
    case PIECE_KING:
        LoadHelper(pieceName, sizeof pieceName, "king", team, row, col, type);
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
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type)
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

/**
 * displayPieces (static)
 *
 * Draw all loaded piece textures stored in GameBoard. If a cell's piece type
 * is PIECE_NONE it is skipped. Uses DrawTextureEx to place the texture at the
 * precomputed GameBoard[row][col].pos position.
 *
 */
static void displayPieces(void)
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
}

/**
 * InitializeCellsPos (static)
 *
 * Compute and store the top-left position (Vector2) for each board cell in GameBoard.
 *
 * Parameters:
 *  - extra: horizontal offset to center the board
 *  - squareLength: size of each square in pixels
 *  - spaceText: fractional extra space used when computing board layout (SPACE_TEXT)
 *
 * Calling pattern:
 *  - Compute squareLength and extra in DrawBoard (or main), then call this to set positions.
 */
static void InitializeCellsPos(int extra, int squareLength, float spaceText)
{
    // row = y, col = x
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].pos = (Vector2){
                (float)extra + ((float)squareLength * spaceText / 2) + ((float)col * (float)squareLength), // x = col
                ((float)row * (float)squareLength) + ((float)squareLength * spaceText / 2)                 // y = row
            };
        }
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
    float squareCount = BOARD_SIZE + SPACE_TEXT;
    return (int)((float)Min2(GetRenderWidth(), GetRenderHeight()) / squareCount);
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
        if (GameBoard[CellX][CellY].piece.type != PIECE_NONE && TurnValidation)
        {
            selectedPiece = GameBoard[CellX][CellY];
            selectedPiece.selected = true;
            SetCellBorder(&selectedCellBorder, &selectedPiece);
            TraceLog(LOG_DEBUG, "Selected A new Piece: %d %d", CellX, CellY);
            PrimaryValidation(selectedPiece.piece.type, CellX, CellY); // Renamed Validateanddevalidate to just validate
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
        if (NewCellX == CellX && NewCellY == CellY)
        {
            selectedPiece.selected = false;
            ResetCellBorder(&selectedCellBorder);
            ResetValidation();
            ResetPrimaryValidation();
            selectedPiece = imaginaryCell;
            return;
        }
        else
        {
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
 *    a small filled circle centered in that square using VALID_MOVE_COLOR.
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

        for (row = 0; row < BOARD_SIZE; row++)
        {
            for (col = 0; col < BOARD_SIZE; col++)
            {
                Cell thisCell = GameBoard[row][col];
                if (thisCell.isvalid)
                {
                    DrawCircle((int)thisCell.pos.x + halfSquareLength, (int)thisCell.pos.y + halfSquareLength, (float)validMoveCircleRadius, VALID_MOVE_COLOR);
                }
            }
        }
    }
}
