/**
 * move.c
 *
 * Responsibilities:
 * - Execute piece movement between cells on the GameBoard.
 * - Provide a helper to clear a cell's piece data and release associated texture.
 *
 * Notes:
 * - These functions operate directly on the global GameBoard array (declared in main.c).
 * - Textures are managed with raylib's LoadTexture/UnloadTexture APIs; callers must ensure
 *   proper sizing/assignment (LoadPiece is used to place textures on destination squares).
 * - All operations are intended to be called from the main thread.
 */

#include "main.h"
#include "draw.h"
#include "move.h"

extern Cell GameBoard[8][8];

// Local Variables
int i, j;
/**
 * MovePiece
 *
 * Move a piece from an initial board square to a final board square.
 *
 * Behavior / Side effects:
 * - Loads the moving piece's texture into the destination cell via LoadPiece().
 * - Marks the destination piece as having moved (hasMoved = 1).
 * - Clears the source cell by calling SetEmptyCell(), which also unloads any texture
 *   that remained on the source.
 *
 * Parameters:
 *  - initialRow, initialCol : source coordinates (0..7)
 *  - finalRow, finalCol     : destination coordinates (0..7)
 *
 * Preconditions / Safety:
 * - This function assumes GameBoard[initialRow][initialCol].piece.type is valid.
 */
void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol)
{
    /* Bounds checking */
    if (initialRow < 0 || initialRow >= 8 || initialCol < 0 || initialCol >= 8 ||
        finalRow < 0 || finalRow >= 8 || finalCol < 0 || finalCol >= 8)
    {
        TraceLog(LOG_WARNING, "MovePiece: indices out of bounds (%d,%d)->(%d,%d)", initialRow, initialCol, finalRow, finalCol);
        return;
    }

    /* Ensure there is a piece at the source */
    if (GameBoard[initialRow][initialCol].piece.type == PIECE_NONE)
    {
        TraceLog(LOG_WARNING, "MovePiece: no piece at source (%d,%d)", initialRow, initialCol);
        return;
    }
    if (initialRow == finalRow && initialCol == finalCol) // This fixes the capturing self bug
    {
        return;
    }
    else
    {
        LoadPiece(finalRow, finalCol, GameBoard[initialRow][initialCol].piece.type, GameBoard[initialRow][initialCol].piece.team);
        GameBoard[finalRow][finalCol].piece.hasMoved = 1;
        SetEmptyCell(&GameBoard[initialRow][initialCol]);
    }
}

/**
 * SetEmptyCell
 *
 * Clear a Cell to represent an empty square and release any associated texture.
 *
 * Behavior / Side effects:
 * - Sets piece.type to PIECE_NONE and resets piece-related flags (hasMoved, enPassant).
 * - Resets piece.team to TEAM_WHITE as a neutral default.
 * - If a Texture2D is present (texture.id != 0) it is UnloadTexture()'d to free GPU memory.
 *
 * Parameters:
 *  - cell: pointer to the Cell to clear (must be non-NULL).
 *
 * Notes:
 * - Safe to call on already-empty cells; UnloadTexture will not be called when texture.id == 0.
 */
void SetEmptyCell(Cell *cell)
{
    cell->piece.type = PIECE_NONE;
    cell->piece.hasMoved = 0;
    cell->piece.enPassant = 0;
    cell->piece.team = TEAM_WHITE;
    cell->piece.texture.id = 0; // Unload texture function was causing very weird behaviour had to replace it with this
}

void ValidateAndDevalidateMoves(PieceType Piece, int CellX, int CellY, int ColorTheme, bool selected)
{
    if (selected)
    {
        switch (Piece)
        {
        case PIECE_KING:
            KingValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_QUEEN:
            QueenValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_ROOK:
            RookValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_BISHOP:
            BishopValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_KNIGHT:
            KnightValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_PAWN:
            PawnValidation(CellX, CellY, ColorTheme);
            break;
        case PIECE_NONE:

            break;
        }
    }
    else // devalidate moves
    {
        Resetvalidation();
    }
}
void KingValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;
    for (i = (CellX - 1); i <= CellX + 1; i++)
    {
        for (j = CellY - 1; j <= CellY + 1; j++)
        {
            if (i >= 0 && i < 8 && j >= 0 && j < 8 && (i != CellX || j != CellY))
            {
                HighlightSquare(i, j, ColorTheme);
                GameBoard[i][j].isvalid = true;
            }
        }
    }
}
void QueenValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;
    RookValidation(CellX, CellY, ColorTheme);
    BishopValidation(CellX, CellY, ColorTheme);
}
void RookValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;
    for (i = 0; i < 8; i++)
    {
        if (i != CellY && i != CellX)
        {
            GameBoard[CellX][i].isvalid = true;
            GameBoard[i][CellY].isvalid = true;
            HighlightSquare(CellX, i, ColorTheme);
            HighlightSquare(i, CellY, ColorTheme);
        }
    }
}
void BishopValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;

    for (i = CellX + 1; i < 8; i++)
    {
        if (CellY - (i - CellX) >= 0)
        {
            HighlightSquare(i, CellY - (i - CellX), ColorTheme);
            GameBoard[i][CellY - (i - CellX)].isvalid = true;
        }
        if (CellY + (i - CellX) < 8)
        {
            HighlightSquare(i, CellY + (i - CellX), ColorTheme);
            GameBoard[i][CellY + (i - CellX)].isvalid = true;
        }
    }
    for (i = CellX - 1; i >= 0; i--)
    {
        if (CellY + (i - CellX) >= 0)
        {
            HighlightSquare(i, CellY + (i - CellX), ColorTheme);
            GameBoard[i][CellY + (i - CellX)].isvalid = true;
        }
        if (CellY - (i - CellX) < 8)
        {
            HighlightSquare(i, CellY - (i - CellX), ColorTheme);
            GameBoard[i][CellY - (i - CellX)].isvalid = true;
        }
    }
}
void KnightValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;
    int row1 = CellX + 2,
        row2 = CellX - 2,
        col1 = CellY + 2,
        col2 = CellY - 2;
    if (row1 < 8)
    {
        if (col2 + 1 >= 0)
        {
            GameBoard[row1][col2 + 1].isvalid = true;
            HighlightSquare(row1, col2 + 1, ColorTheme);
        }
        if (col1 - 1 < 8)
        {
            GameBoard[row1][col1 - 1].isvalid = true;
            HighlightSquare(row1, col1 - 1, ColorTheme);
        }
    }

    if (row2 >= 0)
    {
        if (col2 + 1 >= 0)
        {
            GameBoard[row2][col2 + 1].isvalid = true;
            HighlightSquare(row2, col2 + 1, ColorTheme);
        }
        if (col1 - 1 < 8)
        {
            GameBoard[row2][col1 - 1].isvalid = true;
            HighlightSquare(row2, col1 - 1, ColorTheme);
        }
    }
    if (col1 < 8)
    {
        if (row2 + 1 >= 0)
        {
            GameBoard[row2 + 1][col1].isvalid = true;
            HighlightSquare(row2 + 1, col1, ColorTheme);
        }
        if (row1 - 1 < 8)
        {
            GameBoard[row1 - 1][col1].isvalid = true;
            HighlightSquare(row1 - 1, col1, ColorTheme);
        }
    }

    if (col2 >= 0)
    {
        if (row2 + 1 >= 0)
        {
            GameBoard[row2 + 1][col2].isvalid = true;
            HighlightSquare(row2 + 1, col2, ColorTheme);
        }
        if (row1 - 1 < 8)
        {
            GameBoard[row1 - 1][col2].isvalid = true;
            HighlightSquare(row1 - 1, col2, ColorTheme);
        }
    }
}

void PawnValidation(int CellX, int CellY, int ColorTheme)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    int team = GameBoard[CellX][CellY].piece.team;
    if (team == TEAM_BLACK)
    {
        GameBoard[CellX + 1][CellY].isvalid = true;
        HighlightSquare(CellX + 1, CellY, ColorTheme);
        if (!moved)
        {
            GameBoard[CellX + 2][CellY].isvalid = true;
            HighlightSquare(CellX + 2, CellY, ColorTheme);
        }
    }
    if (team == TEAM_WHITE)
    {
        GameBoard[CellX - 1][CellY].isvalid = true;
        HighlightSquare(CellX - 1, CellY, ColorTheme);
        if (!moved)
        {
            GameBoard[CellX - 2][CellY].isvalid = true;
            HighlightSquare(CellX - 2, CellY, ColorTheme);
        }
    }
}
