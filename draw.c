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
 *     Renders the board and all pieces. Should be called each frame inside
 *     BeginDrawing()/EndDrawing(). Computes layout for the current window size
 *     and calls display routines.
 *
 * - void LoadPiece(int row, int col, PieceType type, Team team, int squareLength);
 *     Loads a piece image from assets/ and assigns the resulting Texture2D to
 *     GameBoard[row][col].piece.texture. If a texture already exists in that
 *     cell it is UnloadTexture()'d before assignment. Caller must ensure row/col
 *     are in [0,7]. `squareLength` controls image resize dimensions.
 *
 * - int ComputeSquareLength(void);
 *     Returns the computed size (in pixels) of a single board square using the
 *     current render width/height. Useful to compute positions/resources from
 *     main after InitWindow() or after a resize event.
 *
 * Notes / conventions:
 * - Filenames are generated as "assets/<piece><W|B>.png" (example: assets/kingW.png).
 * - TrimTrailingWhitespace removes trailing whitespace (useful if names come from
 *   user input). On Linux trailing spaces are significant in filenames so avoid them.
 * - LoadPiece will log warnings on failure (TraceLog). After LoadPiece returns,
 *   check GameBoard[row][col].piece.texture.id to confirm success (non-zero).
 * - This module uses static (file-local) helper functions. None of them are
 *   thread-safe; all operations are expected to be called from the main thread.
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

// This constant determines How much space is left for the text in terms of squareLength
#define SPACETEXT 0.5f

/**
 * DrawBoard
 *
 * Compute layout based on current render size and draw the board and pieces.
 *
 * Parameters:
 *  - ColorTheme: index into PALETTE (colors.h)
 *
 * Behavior:
 *  - Calls ComputeSquareLength() to obtain square size (in pixels).
 *  - Initializes cell positions (InitializeCellsPos) using the computed values.
 *  - Draws the 8x8 board using the chosen ColorPair.
 *  - Calls displayPieces() to render piece textures at computed positions.
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

    displayPieces();
}

/**
 * LoadPiece
 *
 * Public helper to load a piece texture and store it in GameBoard[row][col].
 * It selects the filename by piece type and team and delegates to LoadHelper.
 *
 * Parameters:
 *  - row, col : board coordinates (0..7)
 *  - type     : PieceType enum
 *  - team     : TEAM_WHITE or TEAM_BLACK
 *  - squareLength : desired texture dimension (square)
 *
 * Safety:
 *  - Performs bounds check on row/col.
 *  - LoadHelper handles logging and texture assignment.
 */
void LoadPiece(int row, int col, PieceType type, Team team)
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
        return;

    char pieceName[64];

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
 *  - pieceNameBuffer, bufferSize: output buffer for the generated filename
 *  - pieceName: base name (e.g. "king", "pawn")
 *  - team: TEAM_WHITE/TEAM_BLACK
 *  - squareLength: resize dimension
 *  - row/col: target cell coordinates
 *  - type: PieceType to set on the cell
 */
static void LoadHelper(char *pieceNameBuffer, int bufferSize, const char *pieceName, Team team, int row, int col, PieceType type)
{
    /*This function loads the texture for any given piece correctly and handles errors
    and puts a new texture if one already exists at this cell*/

    int n = snprintf(pieceNameBuffer, (size_t)bufferSize, "assets/%s%c.png", pieceName, (team == TEAM_WHITE) ? 'W' : 'B');
    if (n < 0)
        return;
    if (n >= bufferSize)
        TraceLog(LOG_WARNING, "Filename was truncated: %s", pieceNameBuffer);

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
        UnloadTexture(GameBoard[row][col].piece.texture);

    // Add the piece to the GameBoard
    GameBoard[row][col].piece.texture = texture;
    GameBoard[row][col].piece.type = type;
    GameBoard[row][col].piece.team = team;
    GameBoard[row][col].row = row;
    GameBoard[row][col].col = col;
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
                DrawTextureEx(GameBoard[row][col].piece.texture, GameBoard[row][col].pos, 0, (float)ComputeSquareLength() / GameBoard[row][col].piece.texture.width, WHITE);
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
 *  - extra: horizontal offset to center the board
 *  - squareLength: size of each square in pixels
 *  - spaceText: fractional extra space used when computing board layout (SPACETEXT)
 *
 * Calling pattern:
 *  - Compute squareLength and extra in DrawBoard (or main), then call this to set positions.
 */
static void InitializeCellsPos(int extra, int squareLength, float spaceText)
{
    // row = y, col = x
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            GameBoard[row][col].pos = (Vector2){
                extra + squareLength * spaceText / 2 + col * squareLength, // x = col
                row * squareLength + squareLength * spaceText / 2          // y = row
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
 *  - new length (size_t) after trimming.
 */
static size_t TrimTrailingWhitespace(char *s)
{
    // Check to see if it's an empty sting
    if (!s)
        return 0;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        len--;
    s[len] = '\0';
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

/**
 * ComputeSquareLength
 *
 * Public helper to compute a reasonable square size given the current render
 * width/height and the SPACETEXT constant. This is useful to compute positions
 * or to pass to LoadPiece from main after InitWindow() so that resource sizing
 * and layout logic agree.
 */
int ComputeSquareLength()
{
    float squareCount = 8 + SPACETEXT;
    return Min2(GetRenderWidth(), GetRenderHeight()) / squareCount;
}
