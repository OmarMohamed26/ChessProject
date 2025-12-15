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
#include "colors.h"

extern Cell GameBoard[8][8];
extern Player Player1;
bool flag = false;
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
        Turn = (Turn == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE; // Added turns
        ResetVulnerable();
        ScanEnemyMoves(); // Added vulnerability function checker
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

void MoveValidation(int CellX, int CellY, PieceType type, Team team, bool moved)
{
    int i;

    if (type == PIECE_ROOK)
    {
        for (i = CellX + 1; i < 8; i++)
            if (HandleLinearSquare(i, CellY, team))
                break;
        for (i = CellX - 1; i >= 0; i--)
            if (HandleLinearSquare(i, CellY, team))
                break;
        for (i = CellY + 1; i < 8; i++)
            if (HandleLinearSquare(CellX, i, team))
                break;
        for (i = CellY - 1; i >= 0; i--)
            if (HandleLinearSquare(CellX, i, team))
                break;
    }

    if (type == PIECE_BISHOP)
    {
        for (i = 1; CellX + i < 8 && CellY + i < 8; i++)
            if (HandleLinearSquare(CellX + i, CellY + i, team))
                break;
        for (i = 1; CellX + i < 8 && CellY - i >= 0; i++)
            if (HandleLinearSquare(CellX + i, CellY - i, team))
                break;
        for (i = 1; CellX - i >= 0 && CellY + i < 8; i++)
            if (HandleLinearSquare(CellX - i, CellY + i, team))
                break;
        for (i = 1; CellX - i >= 0 && CellY - i >= 0; i++)
            if (HandleLinearSquare(CellX - i, CellY - i, team))
                break;
    }

    if (type == PIECE_QUEEN)
    {
        for (i = CellX + 1; i < 8; i++)
            if (HandleLinearSquare(i, CellY, team))
                break;
        for (i = CellX - 1; i >= 0; i--)
            if (HandleLinearSquare(i, CellY, team))
                break;
        for (i = CellY + 1; i < 8; i++)
            if (HandleLinearSquare(CellX, i, team))
                break;
        for (i = CellY - 1; i >= 0; i--)
            if (HandleLinearSquare(CellX, i, team))
                break;
        for (i = 1; CellX + i < 8 && CellY + i < 8; i++)
            if (HandleLinearSquare(CellX + i, CellY + i, team))
                break;
        for (i = 1; CellX + i < 8 && CellY - i >= 0; i++)
            if (HandleLinearSquare(CellX + i, CellY - i, team))
                break;
        for (i = 1; CellX - i >= 0 && CellY + i < 8; i++)
            if (HandleLinearSquare(CellX - i, CellY + i, team))
                break;
        for (i = 1; CellX - i >= 0 && CellY - i >= 0; i++)
            if (HandleLinearSquare(CellX - i, CellY - i, team))
                break;
    }

    if (type == PIECE_PAWN)
        HandlePawnMove(CellX, CellY, team, moved);

    if (type == PIECE_KNIGHT)
        HandleKnightMove(CellX, CellY, team);

    if (type == PIECE_KING)
        HandleKingMove(CellX, CellY, team);
}

void ResetValidation()
{
    int i, j;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            GameBoard[i][j].primaryvalid = false; // Added this for future Check and Checkmate validation
            GameBoard[i][j].isvalid = false;
        }
    }
}
void ResetVulnerable()
{
    int i, j;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            GameBoard[i][j].vulnerable = false;
        }
    }
}

void ValidateMoves(PieceType Piece, int CellX, int CellY, bool selected)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    Team team = GameBoard[CellX][CellY].piece.team;
    if (selected)
    {
        switch (Piece)
        {
        case PIECE_KING:
            MoveValidation(CellX, CellY, PIECE_KING, team, moved);
            break;
        case PIECE_QUEEN:
            MoveValidation(CellX, CellY, PIECE_QUEEN, team, moved);
            break;
        case PIECE_ROOK:
            MoveValidation(CellX, CellY, PIECE_ROOK, team, moved);
            break;
        case PIECE_BISHOP:
            MoveValidation(CellX, CellY, PIECE_BISHOP, team, moved);
            break;
        case PIECE_KNIGHT:
            MoveValidation(CellX, CellY, PIECE_KNIGHT, team, moved);
            break;
        case PIECE_PAWN:
            MoveValidation(CellX, CellY, PIECE_PAWN, team, moved);
            break;
        case PIECE_NONE:
            break;
        }
    }
}

void ScanEnemyMoves()
{
    int i, j;

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (GameBoard[i][j].piece.team == Turn || GameBoard[i][j].piece.type == PIECE_NONE)
            {
                continue;
            }

            else
            {
                MoveValidation(i, j, GameBoard[i][j].piece.type, GameBoard[i][j].piece.team, GameBoard[i][j].piece.hasMoved);
            }
        }
    }
}

bool HandleLinearSquare(int x, int y, Team team)
{
    if (GameBoard[x][y].piece.type == PIECE_NONE)
    {
        if (Turn == team)
            GameBoard[x][y].primaryvalid = true;
        return false;
    }
    else if (GameBoard[x][y].piece.team != team)
    {
        if (Turn == team)
            GameBoard[x][y].primaryvalid = true;
        else
            GameBoard[x][y].vulnerable = true;

        return true;
    }
    else
    {
        return true;
    }
}
// Pawn move helper
void HandlePawnMove(int CellX, int CellY, Team team, bool moved)
{
    if (team == TEAM_BLACK && CellX + 1 < 8)
    {
        if (GameBoard[CellX + 1][CellY].piece.type == PIECE_NONE)
        {
            if (Turn == team)
                GameBoard[CellX + 1][CellY].primaryvalid = true;

            if (!moved && CellX + 2 < 8)
            {
                if (Turn == team)
                    GameBoard[CellX + 2][CellY].primaryvalid = true;
            }
        }
        if (GameBoard[CellX + 1][CellY + 1].piece.type != PIECE_NONE && GameBoard[CellX + 1][CellY + 1].piece.team != team)
        {
            if (Turn == team)
                GameBoard[CellX + 1][CellY + 1].primaryvalid = true;
            else
                GameBoard[CellX + 1][CellY + 1].vulnerable = true;
        }
    }
    else if (team == TEAM_WHITE && CellX - 1 >= 0)
    {
        if (GameBoard[CellX - 1][CellY].piece.type == PIECE_NONE)
        {
            if (Turn == team)
                GameBoard[CellX - 1][CellY].primaryvalid = true;

            if (!moved && CellX - 2 >= 0)
            {
                if (Turn == team)
                    GameBoard[CellX - 2][CellY].primaryvalid = true;
            }
        }
        if (GameBoard[CellX - 1][CellY - 1].piece.type != PIECE_NONE && GameBoard[CellX - 1][CellY - 1].piece.team != team)
        {
            if (Turn == team)
                GameBoard[CellX - 1][CellY - 1].primaryvalid = true;
            else
                GameBoard[CellX - 1][CellY - 1].vulnerable = true;
        }
    }
}

void HandleKnightSquare(int x, int y, Team team)
{
    if (x >= 0 && x < 8 && y >= 0 && y < 8)
        if (GameBoard[x][y].piece.team != team || GameBoard[x][y].piece.type == PIECE_NONE)
        {
            if (Turn == team)
                (GameBoard[x][y].primaryvalid = true);
            else if (GameBoard[x][y].piece.type != PIECE_NONE)
            {
                (GameBoard[x][y].vulnerable = true);
            }
        }
}

void HandleKnightMove(int CellX, int CellY, Team team)
{
    int row1 = CellX + 2, row2 = CellX - 2;
    int col1 = CellY + 2, col2 = CellY - 2;

    HandleKnightSquare(row1, col2 + 1, team);
    HandleKnightSquare(row1, col1 - 1, team);
    HandleKnightSquare(row2, col2 + 1, team);
    HandleKnightSquare(row2, col1 - 1, team);
    HandleKnightSquare(row2 + 1, col1, team);
    HandleKnightSquare(row1 - 1, col1, team);
    HandleKnightSquare(row2 + 1, col2, team);
    HandleKnightSquare(row1 - 1, col2, team);
}

void HandleKingMove(int CellX, int CellY, Team team)
{
    int i, j;
    for (i = CellX - 1; i <= CellX + 1; i++)
    {
        for (j = CellY - 1; j <= CellY + 1; j++)
        {
            if (i >= 0 && i < 8 && j >= 0 && j < 8 && (i != CellX || j != CellY))
            {
                if (GameBoard[i][j].piece.team != team || GameBoard[i][j].piece.type == PIECE_NONE)
                {
                    if (Turn == team)
                        (GameBoard[i][j].primaryvalid = true);
                    else if (GameBoard[i][j].piece.type != PIECE_NONE)
                    {
                        (GameBoard[i][j].vulnerable = true);
                    }
                }
            }
        }
    }
}
