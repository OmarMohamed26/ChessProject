/**
 * draw.c
 *
 * Responsibilities:
 * - Compute board layout and cell positions based on current window size.
 * - Render the chess board and pieces.
 * - Load piece images from disk, convert to Texture2D and assign them to GameBoard cells.
 *
 * Public functions (exported in draw.h):
 * - void DrawBoard(int ColorTheme);
 *   Renders the board and all pieces. Should be called each frame inside
 *   BeginDrawing()/EndDrawing(). Computes layout for the current window size
 *   and calls display routines.
 *
 * - void LoadPiece(int row, int col, PieceType type, Team team, int squareLength);
 *   Loads a piece image from assets/ and assigns the resulting Texture2D to
 *   GameBoard[row][col].piece.texture. If a texture already exists in that
 *   cell it is UnloadTexture()'d before assignment. Caller must ensure row/col
 *   are in [0,7]. `squareLength` controls image resize dimensions.
 *
 * - int ComputeSquareLength(void);
 *   Returns the computed size (in pixels) of a single board square using the
 *   current render width/height. Useful to compute positions/resources from
 *   main after InitWindow() or after a resize event.
 *
 * Notes / conventions:
 * - Filenames are generated as "assets/<piece><W|B>.png" (example: assets/kingW.png).
 * - TrimTrailingWhitespace removes trailing whitespace (useful if names come from
 *  user input). On Linux trailing spaces are significant in filenames so avoid them.
 * - LoadPiece will log warnings on failure (TraceLog). After LoadPiece returns,
 *  check GameBoard[row][col].piece.texture.id to confirm success (non-zero).
 * - This module uses static (file-local) helper functions. None of them are
 *  thread-safe; all operations are expected to be called from the main thread.
 *
 * Change minimalism:
 * - This file only adds documentation/comments; no functional changes are made.
 */

#include <raylib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include "colors.h"
#include "draw.h"
#include "main.h"

// access the GameBoard from the main.c file
extern Cell GameBoard[8][8];

// Local Prototypes
static int Min2(int x, int y);
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type);
static void InitializeCellsPos(int extra, int squareLength, float spaceText);
static size_t TrimTrailingWhitespace(char *s);
static void displayPieces(void);
static int ComputeSquareLength();

// This constant determines How much space is left for the text in terms of squareLength
#define SPACETEXT 0.75f

// setting flag values
int pointer = 0;

// indecies of board at click (i1=row, i2=col)
int i1, i2;

//----------------------------------------------------------------------------------
// Drawing and Layout Functions
//----------------------------------------------------------------------------------

/**
 * DrawBoard
 *
 * Compute layout based on current render size and draw the board and pieces.
 *
 * Parameters:
 * - ColorTheme: index into PALETTE (colors.h)
 *
 * Behavior:
 * - Calls ComputeSquareLength() to obtain square size (in pixels).
 * - Initializes cell positions (InitializeCellsPos) using the computed values.
 * - Draws the 8x8 board using the chosen ColorPair.
 * - Calls displayPieces() to render piece textures at computed positions.
 */
void DrawBoard(int ColorTheme)
{
    ColorPair theme = PALETTE[ColorTheme];
    float squareCount = 8 + SPACETEXT;
    int squareLength = ComputeSquareLength();
    int extra = (GetRenderWidth() - squareCount * squareLength) / 2;

    InitializeCellsPos(extra, squareLength, SPACETEXT);

    // Draw the chess board (row = y, col = x)
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Color colr = ((row + col) & 1) ? theme.black : theme.white;
            DrawRectangleV(GameBoard[row][col].pos, (Vector2){squareLength, squareLength}, colr);
        }
    }

    // compute once (matches InitializeCellsPos math)
    float boardLeft = extra + squareLength * SPACETEXT / 2.0f;
    float boardTop = squareLength * SPACETEXT / 2.0f;

    // Draw rank numbers (left) and file letters (bottom), centered in each square.
    int fontSize = (int)(squareLength * 0.25f);
    if (fontSize < 10)
        fontSize = 10;
    if (fontSize > squareLength)
        fontSize = squareLength;

    for (int row = 0; row < 8; row++)
    {
        char rankText[2] = {(char)('8' - row), '\0'};
        int w = MeasureText(rankText, fontSize);
        float x = boardLeft - (float)w - ((float)fontSize * 0.25f); // small gap
        float y = GameBoard[row][0].pos.y + (squareLength - fontSize) * 0.5f;
        DrawText(rankText, (int)x, (int)y, fontSize, FONT_COLOR);
    }

    for (int col = 0; col < 8; col++)
    {
        char fileText[2] = {(char)('a' + col), '\0'};
        int w = MeasureText(fileText, fontSize);
        float x = GameBoard[7][col].pos.x + (squareLength - w) * 0.5f;
        float y = (int)(boardTop + 8 * (float)squareLength + (fontSize * 0.25f)); // below board
        DrawText(fileText, (int)x, (int)y, fontSize, FONT_COLOR);
    }

    displayPieces();
}

/**
 * displayPieces (static)
 *
 * Draw all loaded piece textures stored in GameBoard. If a cell's piece type
 * is PIECE_NONE it is skipped. Uses DrawTextureV to place the texture at the
 * precomputed GameBoard[row][col].pos position.
 *
 */
static void displayPieces(void)
{
    // Draw pieces using same row/col ordering
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (GameBoard[row][col].piece.type != PIECE_NONE)
            {
                // Draw the texture scaled to fit the square length
                DrawTextureEx(GameBoard[row][col].piece.texture, GameBoard[row][col].pos, 0, (float)ComputeSquareLength() / GameBoard[row][col].piece.texture.width, WHITE);
            }
        }
    }
}

/**
 * Highlight_Square
 *
 * Redraws a specific square with a slightly lighter background color to indicate selection or highlight.
 * It also redraws the piece on top to prevent the highlight from obscuring it.
 *
 * Parameters:
 * - row, col: board coordinates (0..7) of the square to highlight.
 * - ColorTheme: index into PALETTE (colors.h) to get base colors.
 */
void Highlight_Square(int row, int col, int ColorTheme)
{
    ColorPair theme = PALETTE[ColorTheme];
    int squareLength = ComputeSquareLength();
    Color colr = ((row + col) & 1) ? theme.black : theme.white;
    // Increase RGB values by 30, clamping at 255 for a lighter shade
    colr.r = (colr.r + 30 <= 255) ? colr.r + 30 : 255;
    colr.g = (colr.g + 30 <= 255) ? colr.g + 30 : 255;
    colr.b = (colr.b + 30 <= 255) ? colr.b + 30 : 255;
    DrawRectangleV(GameBoard[row][col].pos, (Vector2){squareLength, squareLength}, colr);
    DrawTextureEx(GameBoard[row][col].piece.texture, GameBoard[row][col].pos, 0, (float)ComputeSquareLength() / GameBoard[row][col].piece.texture.width, WHITE);
}

/**
 * Highlight_Hover
 *
 * Checks if the mouse cursor is over a piece on the board and highlights the square if so.
 * It also updates the global variables i1 (row) and i2 (col) to the hovered square's index.
 * Changes the mouse cursor to a pointing hand if a piece is under the cursor.
 *
 * Parameters:
 * - ColorTheme: index into PALETTE (colors.h).
 *
 * Side effects:
 * - Updates global variables i1, i2 (hovered row/col).
 * - Calls Highlight_Square() to draw the highlight.
 * - Updates GameBoard[i1][i2].selected if the left mouse button is pressed.
 * - Changes the mouse cursor using SetMouseCursor().
 */
void Highlight_Hover(int ColorTheme)
{
    int Sql = ComputeSquareLength();
    int X_Pos = (GetMouseX());
    int Y_Pos = (GetMouseY());
    float ratiox, ratioy;
    // Calculate the maximum X and Y coordinates that define the board boundaries
    float Max_Board_X = (GameBoard[0][7].pos.x + Sql);
    float Max_Board_Y = (GameBoard[7][0].pos.y + Sql);
    SetMouseCursor(MOUSE_CURSOR_ARROW);
    // Check if the mouse is within the board's drawing area
    if (X_Pos >= GameBoard[0][0].pos.x && X_Pos <= Max_Board_X &&
        Y_Pos >= GameBoard[0][0].pos.y && Y_Pos <= Max_Board_Y)
    {
        // Calculate the column index (i2) based on mouse X position relative to the board
        ratiox = ((X_Pos - GameBoard[0][0].pos.x) * 8) / (Max_Board_X - GameBoard[0][0].pos.x);
        i2 = (int)ratiox;
        // Calculate the row index (i1) based on mouse Y position relative to the board
        ratioy = ((Y_Pos - GameBoard[0][0].pos.y) * 8) / (Max_Board_Y - GameBoard[0][0].pos.y);
        i1 = (int)ratioy;
        // Check if the cell at the calculated index has a piece (texture.id != 0)
        if (i1 >= 0 && i1 < 8 && i2 >= 0 && i2 < 8 && GameBoard[i1][i2].piece.type != PIECE_NONE)
        {
            Highlight_Square(i1, i2, ColorTheme);
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            // Update the 'selected' flag in the cell structure if the left button is clicked
            GameBoard[i1][i2].selected = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        }
        else
        {
            // If hovering over an empty square, reset i1/i2 if out of bounds due to float conversion
            if (i1 < 0 || i1 > 7 || i2 < 0 || i2 > 7)
            {
                i1 = -1; // Use -1 to indicate not hovering over a valid board square
                i2 = -1;
            }
        }
    }
    else
    {
        i1 = -1; // Reset indices if mouse is outside the board area
        i2 = -1;
    }
}

/**
 * select_piece
 *
 * Handles the logic after a piece has been clicked and selected.
 * Checks the type of the selected piece and executes the appropriate move/action logic.
 *
 * Logic flow:
 * - Executes only if the global 'pointer' flag is set AND the square at [i1][i2] has just been selected (clicked).
 * - Uses a switch statement to branch based on the piece type.
 *
 * Dependencies:
 * - Relies on global variables 'pointer', 'i1', 'i2' and the 'selected' field in the GameBoard cell.
 */
void select_piece()
{
    // Check if the engine is ready to handle a click ('pointer' set) and a selection occurred
    if (pointer && GameBoard[i1][i2].selected)
    {
        switch (GameBoard[i1][i2].piece.type)
        {
        case PIECE_KING:
            /* Logic for King moves/actions */
            break;
        case PIECE_QUEEN:
            /* Logic for Queen moves/actions */
            break;
        case PIECE_ROOK:
            /* Logic for Rook moves/actions */
            break;
        case PIECE_BISHOP:
            /* Logic for Bishop moves/actions */
            break;
        case PIECE_KNIGHT:
            /* Logic for Knight moves/actions */
            break;
        case PIECE_PAWN:
            /* Logic for Pawn moves/actions (including en passant and promotion) */
            break;
        }
        // After processing selection, you would typically reset the 'selected' flag
    }
}

//----------------------------------------------------------------------------------
// Piece Loading Functions
//----------------------------------------------------------------------------------

/**
 * LoadPiece
 *
 * Public helper to load a piece texture and store it in GameBoard[row][col].
 * It selects the filename by piece type and team and delegates to LoadHelper.
 *
 * Parameters:
 * - row, col : board coordinates (0..7)
 * - type   : PieceType enum
 * - team   : TEAM_WHITE or TEAM_BLACK
 * - squareLength : desired texture dimension (square)
 *
 * Safety:
 * - Performs bounds check on row/col.
 * - LoadHelper handles logging and texture assignment.
 */
void LoadPiece(int row, int col, PieceType type, Team team)
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
        return;

    char pieceName[64];

    // Determine the base name of the piece and call the helper function
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
 * - pieceNameBuffer, bufferSize: output buffer for the generated filename
 * - pieceName: base name (e.g. "king", "pawn")
 * - team: TEAM_WHITE/TEAM_BLACK
 * - row/col: target cell coordinates
 * - type: PieceType to set on the cell
 */
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type)
{
    /*This function loads the texture for any given piece correctly and handles errors
    and puts a new texture if one already exists at this cell*/

    // Construct the filename (e.g., "assets/pieces/kingW.png")
    int n = snprintf(pieceNameBuffer, (size_t)bufferSize, "assets/pieces/%s%c.png", pieceName, (team == TEAM_WHITE) ? 'W' : 'B');
    if (n < 0)
        return;
    if (n >= bufferSize)
        TraceLog(LOG_WARNING, "Filename was truncated: %s", pieceNameBuffer);

    TrimTrailingWhitespace(pieceNameBuffer);

    Texture2D texture = LoadTexture(pieceNameBuffer);

    // Check for texture loading failure
    if (texture.id == 0 || texture.width == 0 || texture.height == 0)
    {
        TraceLog(LOG_WARNING, "Failed to load texture: %s", pieceNameBuffer);
        return;
    }

    // Ensure texture is square
    if (texture.width != texture.height)
    {
        TraceLog(LOG_WARNING, "Invalid texture Error, texture's width must equal it's height");
        return;
    }

    // unload previous texture if present
    if (GameBoard[row][col].piece.texture.id != 0)
        UnloadTexture(GameBoard[row][col].piece.texture);

    // Add the piece to the GameBoard
    GameBoard[row][col].piece.texture = texture;
    GameBoard[row][col].piece.type = type;
    GameBoard[row][col].piece.team = team;
}

//----------------------------------------------------------------------------------
// Initialization and Utility Functions
//----------------------------------------------------------------------------------

/**
 * InitializeCellsPos (static)
 *
 * Compute and store the top-left position (Vector2) for each board cell in GameBoard.
 *
 * Parameters:
 * - extra: horizontal offset to center the board
 * - squareLength: size of each square in pixels
 * - spaceText: fractional extra space used when computing board layout (SPACETEXT)
 *
 * Calling pattern:
 * - Compute squareLength and extra in DrawBoard (or main), then call this to set positions.
 */
static void InitializeCellsPos(int extra, int squareLength, float spaceText)
{
    // row = y, col = x
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            GameBoard[row][col].pos = (Vector2){
                extra + squareLength * spaceText / 2 + col * squareLength, // x = col: offset + left margin + column position
                row * squareLength + squareLength * spaceText / 2          // y = row: top margin + row position
            };
        }
    }
}

/**
 * ComputeSquareLength
 *
 * Public helper to compute a reasonable square size given the current render
 * width/height and the SPACETEXT constant. This is useful to compute positions
 * or to pass to LoadPiece from main after InitWindow() so that resource sizing
 * and layout logic agree.
 */
static int ComputeSquareLength()
{
    float squareCount = 8 + SPACETEXT;
    // Square length is determined by the minimum of screen width or height divided by the total number of squares plus margin space.
    return Min2(GetRenderWidth(), GetRenderHeight()) / squareCount;
}

/**
 * InitializeBoard
 *
 * Public function to set all cells in the GameBoard to their initial empty state.
 * Sets coordinates, clears piece data, and resets movement flags.
 */
void InitializeBoard(void)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            GameBoard[i][j].row = i;
            GameBoard[i][j].col = j;
            GameBoard[i][j].piece.hasMoved = 0;
            GameBoard[i][j].piece.enPassant = 0;
            GameBoard[i][j].piece.team = TEAM_WHITE;
            GameBoard[i][j].piece.type = PIECE_NONE;
        }
    }
}

/**
 * UnloadBoard
 *
 * Public function to safely unload all piece textures from the GameBoard
 * and reset piece data to an empty state before closing the window or re-initializing.
 */
void UnloadBoard(void)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            // Check if a texture exists before attempting to unload
            if (GameBoard[i][j].piece.type != PIECE_NONE)
            {
                UnloadTexture(GameBoard[i][j].piece.texture);
            }

            // Reset the cell's piece state
            GameBoard[i][j].piece.type = PIECE_NONE;
            GameBoard[i][j].piece.hasMoved = 0;
            GameBoard[i][j].piece.enPassant = 0;
            GameBoard[j][j].piece.team = TEAM_WHITE;
        }
    }
}

/**
 * TrimTrailingWhitespace (static)
 *
 * Remove trailing whitespace from a NUL-terminated C string in-place and return the new length.
 * Whitespace is tested with isspace((unsigned char)c).
 *
 * Parameters:
 * - s: The string to modify.
 *
 * Returns:
 * - new length (size_t) after trimming.
 */
static size_t TrimTrailingWhitespace(char *s)
{
    // Check to see if it's an empty sting
    if (!s)
        return 0;
    size_t len = strlen(s);
    // Loop backwards, decrementing length until a non-whitespace character is found or the beginning is reached
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        len--;
    s[len] = '\0'; // Null-terminate at the new length
    return len;
}

/**
 * Min2 (static)
 *
 * Return the smaller of two ints.
 */
static int Min2(int x, int y)
{
    return x < y ? x : y;
}