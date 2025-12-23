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

#include "move.h"
#include "draw.h"
#include "main.h"
#include "settings.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>

bool checked = false, flag = false;

/* Add prototypes near the top of the file (below includes) */
static void RaycastRook(int CellX, int CellY, Team team);
static void RaycastBishop(int CellX, int CellY, Team team);

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
    if (initialRow < 0 || initialRow >= BOARD_SIZE || initialCol < 0 || initialCol >= BOARD_SIZE ||
        finalRow < 0 || finalRow >= BOARD_SIZE || finalCol < 0 || finalCol >= BOARD_SIZE)
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
    // before loading piece save the move in struct Move

    // --- ADDED: Update Castling Rights on Rook Capture ---
    // If we capture a rook, the opponent loses castling rights on that side.
    if (GameBoard[finalRow][finalCol].piece.type == PIECE_ROOK)
    {
        if (GameBoard[finalRow][finalCol].piece.team == TEAM_WHITE)
        {
            if (finalRow == WHITE_BACK_RANK && finalCol == ROOK_QS_COL)
            {
                state.whiteQueenSide = false;
            }
            if (finalRow == WHITE_BACK_RANK && finalCol == ROOK_KS_COL)
            {
                state.whiteKingSide = false;
            }
        }
        else if (GameBoard[finalRow][finalCol].piece.team == TEAM_BLACK)
        {
            if (finalRow == BLACK_BACK_RANK && finalCol == ROOK_QS_COL)
            {
                state.blackQueenSide = false;
            }
            if (finalRow == BLACK_BACK_RANK && finalCol == ROOK_KS_COL)
            {
                state.blackKingSide = false;
            }
        }
    }

    // DeadPiece Handling
    if (Turn == TEAM_WHITE)
    {
        if (GameBoard[finalRow][finalCol].piece.type != PIECE_NONE && GameBoard[finalRow][finalCol].piece.team == TEAM_BLACK)
        {
            LoadPiece(deadBlackCounter++, 1 /*This is not important*/, GameBoard[finalRow][finalCol].piece.type, TEAM_BLACK, DEAD_BLACK_PIECES);
        }
    }
    else
    {
        if (GameBoard[finalRow][finalCol].piece.type != PIECE_NONE && GameBoard[finalRow][finalCol].piece.team == TEAM_WHITE)
        {
            LoadPiece(deadWhiteCounter++, 1 /*This is not important*/, GameBoard[finalRow][finalCol].piece.type, TEAM_WHITE, DEAD_WHITE_PIECES);
        }
    }

    // --- CHANGED: Castling Logic ---
    // We now detect castling by checking if the King moved 2 squares horizontally.

    PieceType movingPieceType = GameBoard[initialRow][initialCol].piece.type;

    if (movingPieceType == PIECE_KING && abs(finalCol - initialCol) == 2)
    {
        if (Turn == TEAM_WHITE)
        {
            if (finalRow == WHITE_BACK_RANK && finalCol == CASTLE_KS_KING_COL) // White King Side (g1)
            {
                LoadPiece(WHITE_BACK_RANK, CASTLE_KS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][KING_START_COL]);
                LoadPiece(WHITE_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_KS_COL]);

                state.whiteKingSide = false;
                state.whiteQueenSide = false;
                ResetsAndValidations();
                return;
            }
            if (finalRow == WHITE_BACK_RANK && finalCol == CASTLE_QS_KING_COL) // White Queen Side (c1)
            {
                LoadPiece(WHITE_BACK_RANK, CASTLE_QS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][KING_START_COL]);
                LoadPiece(WHITE_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_QS_COL]);

                state.whiteKingSide = false;
                state.whiteQueenSide = false;
                ResetsAndValidations();
                return;
            }
        }
        else if (Turn == TEAM_BLACK)
        {
            if (finalRow == BLACK_BACK_RANK && finalCol == CASTLE_KS_KING_COL) // Black King Side (g8)
            {
                LoadPiece(BLACK_BACK_RANK, CASTLE_KS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][KING_START_COL]);
                LoadPiece(BLACK_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_KS_COL]);

                state.blackKingSide = false;
                state.blackQueenSide = false;
                ResetsAndValidations();
                return;
            }
            if (finalRow == BLACK_BACK_RANK && finalCol == CASTLE_QS_KING_COL) // Black Queen Side (c8)
            {
                LoadPiece(BLACK_BACK_RANK, CASTLE_QS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][KING_START_COL]);
                LoadPiece(BLACK_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_QS_COL]);

                state.blackKingSide = false;
                state.blackQueenSide = false;
                ResetsAndValidations();
                return;
            }
        }
    }

    // --- Update Castling Rights Flags ---
    // If King or Rook moves normally, we lose castling rights.
    if (movingPieceType == PIECE_KING)
    {
        if (Turn == TEAM_WHITE)
        {
            state.whiteKingSide = false;
            state.whiteQueenSide = false;
        }
        else
        {
            state.blackKingSide = false;
            state.blackQueenSide = false;
        }
    }
    else if (movingPieceType == PIECE_ROOK)
    {
        if (Turn == TEAM_WHITE)
        {
            if (initialRow == WHITE_BACK_RANK && initialCol == ROOK_QS_COL)
            {
                state.whiteQueenSide = false;
            }
            if (initialRow == WHITE_BACK_RANK && initialCol == ROOK_KS_COL)
            {
                state.whiteKingSide = false;
            }
        }
        else
        {
            if (initialRow == BLACK_BACK_RANK && initialCol == ROOK_QS_COL)
            {
                state.blackQueenSide = false;
            }
            if (initialRow == BLACK_BACK_RANK && initialCol == ROOK_KS_COL)
            {
                state.blackKingSide = false;
            }
        }
    }

    // 1. Move the piece
    LoadPiece(finalRow, finalCol, GameBoard[initialRow][initialCol].piece.type, GameBoard[initialRow][initialCol].piece.team, GAME_BOARD);
    GameBoard[finalRow][finalCol].piece.hasMoved = 1;
    SetEmptyCell(&GameBoard[initialRow][initialCol]);

    // --- NEW: Check for Promotion ---
    if (GameBoard[finalRow][finalCol].piece.type == PIECE_PAWN)
    {
        // White reaches row 0, Black reaches row 7 (assuming 0 is top)
        if ((GameBoard[finalRow][finalCol].piece.team == TEAM_WHITE && finalRow == 0) ||
            (GameBoard[finalRow][finalCol].piece.team == TEAM_BLACK && finalRow == BOARD_SIZE - 1))
        {
            state.isPromoting = true;
            state.promotionRow = finalRow;
            state.promotionCol = finalCol;

            // RETURN EARLY: Pause the game, wait for input
            return;
        }
    }

    ResetsAndValidations();
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
    cell->piece.team = TEAM_WHITE;
    // cell->piece.texture.id = 0; // Unload texture function was causing very weird behaviour had to replace it with this
    UnloadTexture(cell->piece.texture);
}

/**
 * MoveValidation
 *
 * Compute primary (geometric) move/raytrace candidates for a piece located at (CellX,CellY).
 *
 * Parameters:
 *  - CellX, CellY : coordinates of the piece to validate.
 *  - type         : PieceType identifying movement rules to apply.
 *  - team         : the piece's Team.
 *  - moved        : whether the piece has moved before (affects pawn and castling logic).
 *
 * Behavior:
 *  - Marks GameBoard squares' primaryValid or vulnerable flags according to geometric reachability
 *    (does not perform final king-checking validation).
 *  - Delegates to piece-specific helpers: rook/bishop/queen cast rays, pawns/knights/kings use helpers.
 *
 * Side effects:
 *  - Mutates GameBoard[*][*].primaryValid and/or .vulnerable.
 */
void MoveValidation(int CellX, int CellY, PieceType type, Team team, bool moved)
{
    if (type == PIECE_ROOK)
    {
        RaycastRook(CellX, CellY, team);
    }

    if (type == PIECE_BISHOP)
    {
        RaycastBishop(CellX, CellY, team);
    }

    if (type == PIECE_QUEEN)
    {
        /* reuse rook + bishop raycasts */
        RaycastRook(CellX, CellY, team);
        RaycastBishop(CellX, CellY, team);
    }

    if (type == PIECE_PAWN)
    {
        HandlePawnMove(CellX, CellY, team, moved);
    }

    if (type == PIECE_KNIGHT)
    {
        HandleKnightMove(CellX, CellY, team);
    }

    if (type == PIECE_KING)
    {
        HandleKingMove(CellX, CellY, team);
    }
}

/**
 * RaycastRook
 *
 * Cast rook-like rays from (CellX,CellY) to mark reachable squares.
 *
 * Parameters:
 *  - CellX, CellY : source coordinates.
 *  - team         : team of the sliding piece.
 *
 * Behavior:
 *  - Iterates in four orthogonal directions (up/down/left/right) until the board edge
 *    or a blocking piece is reached. For each inspected square calls HandleLinearSquare()
 *    which sets primaryValid or vulnerable and returns whether the ray must stop.
 *
 * Side effects:
 *  - Mutates GameBoard[*][*].primaryValid / vulnerable via HandleLinearSquare.
 */
static void RaycastRook(int CellX, int CellY, Team team)
{
    int i;
    for (i = CellX + 1; i < BOARD_SIZE; i++)
    {
        if (HandleLinearSquare(i, CellY, team))
        {
            break;
        }
    }
    for (i = CellX - 1; i >= 0; i--)
    {
        if (HandleLinearSquare(i, CellY, team))
        {
            break;
        }
    }
    for (i = CellY + 1; i < BOARD_SIZE; i++)
    {
        if (HandleLinearSquare(CellX, i, team))
        {
            break;
        }
    }
    for (i = CellY - 1; i >= 0; i--)
    {
        if (HandleLinearSquare(CellX, i, team))
        {
            break;
        }
    }
}

/**
 * RaycastBishop
 *
 * Cast bishop-like diagonal rays from (CellX,CellY) to mark reachable squares.
 *
 * Parameters:
 *  - CellX, CellY : source coordinates.
 *  - team         : team of the sliding piece.
 *
 * Behavior:
 *  - Iterates along the four diagonals until the board edge or a blocking piece.
 *    Uses HandleLinearSquare() for each square which sets primaryValid/vulnerable
 *    and returns whether the ray must stop.
 *
 * Side effects:
 *  - Mutates GameBoard[*][*].primaryValid / vulnerable via HandleLinearSquare.
 */
static void RaycastBishop(int CellX, int CellY, Team team)
{
    int i;
    for (i = 1; CellX + i < BOARD_SIZE && CellY + i < BOARD_SIZE; i++)
    {
        if (HandleLinearSquare(CellX + i, CellY + i, team))
        {
            break;
        }
    }
    for (i = 1; CellX + i < BOARD_SIZE && CellY - i >= 0; i++)
    {
        if (HandleLinearSquare(CellX + i, CellY - i, team))
        {
            break;
        }
    }
    for (i = 1; CellX - i >= 0 && CellY + i < BOARD_SIZE; i++)
    {
        if (HandleLinearSquare(CellX - i, CellY + i, team))
        {
            break;
        }
    }
    for (i = 1; CellX - i >= 0 && CellY - i >= 0; i++)
    {
        if (HandleLinearSquare(CellX - i, CellY - i, team))
        {
            break;
        }
    }
}

/**
 * ResetValidation
 *
 * Clear the per-square final-validation flag (isvalid) across the entire board.
 *
 * Behavior:
 *  - Sets GameBoard[i][j].isvalid = false for all squares.
 */
void ResetValidation()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].isvalid = false;
        }
    }
}

/**
 * ResetVulnerable
 *
 * Clear the per-square vulnerable flag across the entire board.
 *
 * Behavior:
 *  - Sets GameBoard[i][j].vulnerable = false for all squares.
 */
void ResetVulnerable()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].vulnerable = false;
        }
    }
}

/**
 * ResetPrimaryValidation
 *
 * Clear the per-square primary validation flag (primaryValid) for the whole board.
 *
 * Behavior:
 *  - Sets GameBoard[i][j].primaryValid = false for all squares.
 */
void ResetPrimaryValidation()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].primaryValid = false;
        }
    }
}

void ResetMovedStatus()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].hasMoved = false;
        }
    }
}

/**
 * PrimaryValidation
 *
 * Entry point to compute geometric primary-valid moves for a piece at (CellX,CellY).
 *
 * Parameters:
 *  - Piece : PieceType at the source square.
 *  - CellX, CellY : coordinates of the source piece.
 *
 * Behavior:
 *  - Looks up whether the piece has moved and its team, then delegates to MoveValidation.
 *  - Does no simulation/king-check filtering; primaryValid flags represent raw reachable squares.
 */
void PrimaryValidation(PieceType Piece, int CellX, int CellY, bool selected)
{
    bool moved = GameBoard[CellX][CellY].piece.hasMoved;
    Team team = GameBoard[CellX][CellY].piece.team;

    switch (Piece)
    {
    case PIECE_KING:
        MoveValidation(CellX, CellY, PIECE_KING, team, moved);
        if (selected)
        {
            PrimaryCastlingValidation();
        }

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

/**
 * ScanEnemyMoves
 *
 * Compute primary move/vulnerability maps for all opponent pieces (non-Turn).
 *
 * Behavior:
 *  - Iterates the board and runs MoveValidation for every non-empty cell whose team != Turn.
 *  - Results populate primaryValid/vulnerable flags that other code uses for checks.
 *
 * Notes:
 *  - Skips empty squares and pieces on the current Turn side.
 */
void ScanEnemyMoves()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece thisPiece = GameBoard[row][col].piece;

            if (thisPiece.team == Turn || thisPiece.type == PIECE_NONE)
            {
                continue;
            }

            MoveValidation(row, col, thisPiece.type, thisPiece.team, thisPiece.hasMoved);
        }
    }
}

/**
 * ScanFriendlyMoves
 *
 * Compute primary move maps for all friendly pieces (currently on Turn).
 *
 * Behavior:
 *  - Iterates the board and runs MoveValidation for every non-empty cell whose team == Turn.
 *  - Useful for determining legal moves available to the current player.
 *
 * Note:
 *  - This function is currently unused in the codebase but kept for completeness.
 */
void ScanFriendlyMoves()
{
    int row;
    int col;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            Piece thisPiece = GameBoard[row][col].piece;

            if (thisPiece.team != Turn || thisPiece.type == PIECE_NONE)
            {
                continue;
            }

            MoveValidation(row, col, thisPiece.type, thisPiece.team, thisPiece.hasMoved);
        }
    }
}

/**
 * HandleLinearSquare
 *
 * Helper used by sliding pieces (rook/bishop/queen) to process one square along a ray.
 *
 * Parameters:
 *  - row, col : coordinates of the square being inspected.
 *  - team     : team of the sliding piece casting the ray.
 *
 * Returns:
 *  - true  : the ray should stop after this square (square occupied).
 *  - false : the ray may continue (square empty).
 *
 * Side effects:
 *  - Sets GameBoard[row][col].primaryValid if the square is a valid destination for the given team.
 *  - Sets GameBoard[row][col].vulnerable when evaluating vulnerability from the opponent perspective.
 */
bool HandleLinearSquare(int row, int col, Team team)
{
    Cell thisCell = GameBoard[row][col];
    if (thisCell.piece.type == PIECE_NONE)
    {
        if (Turn == team)
        {
            GameBoard[row][col].primaryValid = true;
        }
        else
        {
            GameBoard[row][col].vulnerable = true;
        }
        return false;
    }

    if (thisCell.piece.team != team)
    {
        if (Turn == team)
        {
            GameBoard[row][col].primaryValid = true;
        }
        else
        {
            GameBoard[row][col].vulnerable = true;
        }

        return true;
    }

    if (Turn != team)
    {
        GameBoard[row][col].vulnerable = true;
    }

    return true;
}

/**
 * HandlePawnMove
 *
 * Compute pawn move targets (forward moves and captures) for a pawn at (CellX,CellY).
 *
 * Parameters:
 *  - CellX, CellY : pawn coordinates.
 *  - team         : pawn's team.
 *  - moved        : whether the pawn has moved before (affects two-square advance).
 *
 * Behavior:
 *  - Marks forward destinations as primaryValid when empty.
 *  - Marks diagonal capture squares as primaryValid (if enemy present) or vulnerable when evaluating opponent threats.
 *
 * Notes:
 *  - Does not handle en-passant or promotions specially; en-passant may be handled elsewhere.
 */
void HandlePawnMove(int CellX, int CellY, Team team, bool moved)
{
    if (team == TEAM_BLACK && CellX + 1 < BOARD_SIZE)
    {
        if (GameBoard[CellX + 1][CellY].piece.type == PIECE_NONE)
        {
            if (Turn == team)
            {
                GameBoard[CellX + 1][CellY].primaryValid = true;
            }

            if (!moved && CellX + 2 < BOARD_SIZE)
            {
                if (GameBoard[CellX + 2][CellY].piece.type == PIECE_NONE)
                {
                    if (Turn == team)
                    {
                        GameBoard[CellX + 2][CellY].primaryValid = true;
                    }
                }
            }
        }

        if (GameBoard[CellX + 1][CellY + 1].piece.team != team)
        {
            if (Turn == team && GameBoard[CellX + 1][CellY + 1].piece.type != PIECE_NONE)
            {
                GameBoard[CellX + 1][CellY + 1].primaryValid = true;
            }
            else if (Turn != team)
            {
                GameBoard[CellX + 1][CellY + 1].vulnerable = true;
            }
        }
        if (GameBoard[CellX + 1][CellY - 1].piece.team != team)
        {
            if (Turn == team && GameBoard[CellX + 1][CellY - 1].piece.type != PIECE_NONE)
            {
                GameBoard[CellX + 1][CellY - 1].primaryValid = true;
            }
            else if (Turn != team)
            {
                GameBoard[CellX + 1][CellY - 1].vulnerable = true;
            }
        }
    }
    else if (team == TEAM_WHITE && CellX - 1 >= 0)
    {
        if (GameBoard[CellX - 1][CellY].piece.type == PIECE_NONE)
        {
            if (Turn == team)
            {
                GameBoard[CellX - 1][CellY].primaryValid = true;
            }
            if (!moved && CellX - 2 >= 0)
            {
                if (GameBoard[CellX - 2][CellY].piece.type == PIECE_NONE)
                {
                    if (Turn == team)
                    {
                        GameBoard[CellX - 2][CellY].primaryValid = true;
                    }
                }
            }
        }

        if (GameBoard[CellX - 1][CellY - 1].piece.team != team)
        {
            if (Turn == team && GameBoard[CellX - 1][CellY - 1].piece.type != PIECE_NONE)
            {
                GameBoard[CellX - 1][CellY - 1].primaryValid = true;
            }
            else if (Turn != team)
            {
                GameBoard[CellX - 1][CellY - 1].vulnerable = true;
            }
        }
        if (GameBoard[CellX - 1][CellY + 1].piece.team != team)
        {
            if (Turn == team && GameBoard[CellX - 1][CellY + 1].piece.type != PIECE_NONE)
            {
                GameBoard[CellX - 1][CellY + 1].primaryValid = true;
            }
            else if (Turn != team)
            {
                GameBoard[CellX - 1][CellY + 1].vulnerable = true;
            }
        }
    }
}

/**
 * HandleKnightSquare
 *
 * Helper to mark a single target square for a knight move.
 *
 * Parameters:
 *  - x,y  : target coordinates to consider.
 *  - team : team of the knight performing the move.
 *
 * Behavior:
 *  - If inside the board and target square is empty or enemy, mark primaryValid when evaluating the current player's moves,
 *    otherwise mark vulnerable when evaluating opponent's threats.
 */
void HandleKnightSquare(int row, int col, Team team)
{
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE)
    {
        if (GameBoard[row][col].piece.team != team || GameBoard[row][col].piece.type == PIECE_NONE)
        {
            if (Turn == team)
            {
                (GameBoard[row][col].primaryValid = true);
            }
            else // --- FIX: Removed type check here --- this looks a lot like an AI fix :)
            {
                (GameBoard[row][col].vulnerable = true);
            }
        }
    }
}

/**
 * HandleKnightMove
 *
 * Generate knight move candidates from (CellX,CellY).
 *
 * Parameters:
 *  - CellX, CellY : knight coordinates.
 *  - team         : knight's team.
 *
 * Behavior:
 *  - Calls HandleKnightSquare for all eight knight destinations.
 */
void HandleKnightMove(int CellX, int CellY, Team team)
{
    int row1 = CellX + 2;
    int row2 = CellX - 2;
    int col1 = CellY + 2;
    int col2 = CellY - 2;

    HandleKnightSquare(row1, col2 + 1, team);
    HandleKnightSquare(row1, col1 - 1, team);
    HandleKnightSquare(row2, col2 + 1, team);
    HandleKnightSquare(row2, col1 - 1, team);
    HandleKnightSquare(row2 + 1, col1, team);
    HandleKnightSquare(row1 - 1, col1, team);
    HandleKnightSquare(row2 + 1, col2, team);
    HandleKnightSquare(row1 - 1, col2, team);
}

/**
 * HandleKingMove
 *
 * Generate king move candidates from (CellX,CellY), respecting vulnerable squares.
 *
 * Parameters:
 *  - CellX, CellY : king coordinates.
 *  - team         : king's team.
 *
 * Behavior:
 *  - Examines all adjacent squares; if the square is not marked vulnerable it may be marked primaryValid
 *    (or vulnerable when scanning enemy coverage).
 */
void HandleKingMove(int CellX, int CellY, Team team)
{
    for (int row = CellX - 1; row <= CellX + 1; row++)
    {
        for (int col = CellY - 1; col <= CellY + 1; col++)
        {
            if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && (row != CellX || col != CellY))
            {
                if (!GameBoard[row][col].vulnerable)
                {
                    if (GameBoard[row][col].piece.team != team || GameBoard[row][col].piece.type == PIECE_NONE)
                    {
                        if (Turn == team)
                        {
                            (GameBoard[row][col].primaryValid = true);
                        }
                        else // --- FIX: Removed type check here ---
                        {
                            (GameBoard[row][col].vulnerable = true);
                        }
                    }
                }
            }
        }
    }
}

/**
 * CheckValidation
 *
 * Determine whether the current player's king is under attack and set Player.Checked.
 *
 * Behavior:
 *  - Clears the opponent's Checked flag for the current turn, then locates the current player's king.
 *  - If the king's square has the vulnerable flag set, marks the corresponding Player.Checked.
 */
void CheckValidation()
{
    (Turn == TEAM_WHITE) ? (Player2.Checked = false) : (Player1.Checked = false);

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Cell thisCell = GameBoard[row][col];

            if (thisCell.piece.team == Turn && thisCell.piece.type == PIECE_KING)
            {
                if (thisCell.vulnerable)
                {
                    (Turn == TEAM_WHITE) ? (Player1.Checked = true) : (Player2.Checked = true);
                }

                return;
            }
        }
    }
}

/**
 * FinalValidation
 *
 * From primaryValid candidates compute final legal moves (isvalid) by simulating moves
 * and rejecting those that leave the player's king in check.
 *
 * Parameters:
 *  - CellX, CellY : coordinates of the selected piece.
 *  - selected     : whether a piece is currently selected (only then do final validation).
 *
 * Behavior:
 *  - Iterates primaryValid squares, simulates each move, re-scans enemy moves and checks for simulated check.
 *  - Sets GameBoard[i][j].isvalid true only for moves that do not result in self-check.
 *
 * Side effects:
 *  - Mutates GameBoard[*][*].isvalid and uses MoveSimulation/UndoSimulation repeatedly.
 */
void FinalValidation(int CellX, int CellY, bool selected)
{
    PieceType piece1;
    PieceType piece2;
    Team team2;
    Player1.SimChecked = false;
    Player2.SimChecked = false;

    if (selected)
    {
        piece1 = GameBoard[CellX][CellY].piece.type;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                Cell thisCell = GameBoard[row][col];

                if (thisCell.primaryValid)
                {
                    piece2 = thisCell.piece.type;
                    team2 = thisCell.piece.team;
                    MoveSimulation(CellX, CellY, row, col, piece1);
                    ResetVulnerable();
                    ScanEnemyMoves();
                    SimCheckValidation(); // simulation checkvalidation
                    if (Turn == TEAM_WHITE)
                    {
                        if (Player1.SimChecked)
                        {
                            GameBoard[row][col].isvalid = false;
                        }
                        else
                        {
                            GameBoard[row][col].isvalid = true;
                            // Removed global flag setting (KingSide/QueenSide)
                        }
                    }
                    else
                    {
                        if (Player2.SimChecked)
                        {
                            GameBoard[row][col].isvalid = false;
                        }
                        else
                        {
                            GameBoard[row][col].isvalid = true;
                            // Removed global flag setting (KingSide/QueenSide)
                        }
                    }

                    UndoSimulation(CellX, CellY, row, col, piece1, piece2, team2);
                    ResetVulnerable();
                    ScanEnemyMoves();
                    Player1.SimChecked = false;
                    Player2.SimChecked = false;
                }
            }
        }
    }
}

/**
 * MoveSimulation
 *
 * Perform a lightweight in-place move of piece type between two squares for simulation purposes.
 *
 * Parameters:
 *  - CellX1, CellY1 : source coordinates.
 *  - CellX2, CellY2 : destination coordinates.
 *  - piece           : PieceType being moved.
 *
 * Notes:
 *  - Only updates piece.type and piece.team; does not touch textures or hasMoved.
 *  - Intended for short-lived simulations that will be undone by UndoSimulation.
 */
void MoveSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece)
{
    GameBoard[CellX2][CellY2].piece.team = GameBoard[CellX1][CellY1].piece.team;
    GameBoard[CellX1][CellY1].piece.type = PIECE_NONE;
    GameBoard[CellX2][CellY2].piece.type = piece;
}

/**
 * UndoSimulation
 *
 * Restore board state after a MoveSimulation.
 *
 * Parameters:
 *  - CellX1, CellY1 : source coordinates prior to simulation.
 *  - CellX2, CellY2 : destination coordinates used in simulation.
 *  - piece1          : original source PieceType before simulation.
 *  - piece2          : original destination PieceType before simulation.
 *  - team2           : original destination Team before simulation.
 *
 * Behavior:
 *  - Restores piece types and team for the two involved squares.
 */
void UndoSimulation(int CellX1, int CellY1, int CellX2, int CellY2, PieceType piece1, PieceType piece2, Team team2)
{
    GameBoard[CellX1][CellY1].piece.type = piece1;
    GameBoard[CellX2][CellY2].piece.type = piece2;
    GameBoard[CellX2][CellY2].piece.team = team2;
}

/**
 * CheckmateValidation
 *
 * Run a full search to determine whether a checked player has any legal escapes.
 *
 * Behavior:
 *  - If a player's Checked flag is set, calls CheckmateFlagCheck for that player and sets Player.Checkmated and global Checkmate.
 */
void CheckmateValidation()
{
    if (Player1.Checked)
    {
        Player1.Checkmated = CheckmateFlagCheck(TEAM_WHITE);
        if (Player1.Checkmated)
        {
            Checkmate = true;
        }
    }
    if (Player2.Checked)
    {
        Player2.Checkmated = CheckmateFlagCheck(TEAM_BLACK);
        if (Player2.Checkmated)
        {
            Checkmate = true;
        }
    }
}

/**
 * CheckmateFlagCheck
 *
 * Determine whether playerTeam has any legal move that avoids check.
 *
 * Parameters:
 *  - playerTeam : the Team to analyze.
 *
 * Returns:
 *  - true  : the player has no legal moves that avoid check (checkmate / no legal escape).
 *  - false : the player has at least one legal move that avoids check.
 *
 * Behavior:
 *  - For each piece of playerTeam, computes primaryValid moves, simulates each, and checks whether any move leads
 *    to a position where the king is not in check (using SimCheckValidation).
 */
bool CheckmateFlagCheck(Team playerTeam) // Will also use for stalemate
{
    // I won't change these four variables to row/col I will let it as so
    int i, j, k, l;
    PieceType piece1;
    PieceType piece2;
    bool falsecheck = false;
    Team team1;
    Team team2;
    ResetPrimaryValidation();
    for (i = 0; i < BOARD_SIZE; i++)
    {
        for (j = 0; j < BOARD_SIZE; j++)
        {
            /* code */
            piece1 = GameBoard[i][j].piece.type;
            team1 = GameBoard[i][j].piece.team;

            if (team1 == playerTeam && piece1 != PIECE_NONE)
            {
                PrimaryValidation(piece1, i, j, false);
                for (k = 0; k < BOARD_SIZE; k++)
                {
                    for (l = 0; l < BOARD_SIZE; l++)
                    {
                        if (GameBoard[k][l].primaryValid)
                        {
                            piece2 = GameBoard[k][l].piece.type;
                            team2 = GameBoard[k][l].piece.team;
                            MoveSimulation(i, j, k, l, piece1);
                            ResetVulnerable();
                            ScanEnemyMoves();
                            SimCheckValidation();
                            if (Turn == TEAM_WHITE)
                            {
                                if (!Player1.SimChecked)
                                {
                                    falsecheck = true;
                                }
                            }
                            else
                            {
                                if (!Player2.SimChecked)
                                {
                                    falsecheck = true;
                                }
                            }

                            UndoSimulation(i, j, k, l, piece1, piece2, team2);
                            ResetVulnerable();
                            ScanEnemyMoves();
                            Player1.SimChecked = false;
                            Player2.SimChecked = false;

                            if (falsecheck)
                            {
                                ResetPrimaryValidation();
                                return false;
                            }
                        }
                    }
                }
                ResetPrimaryValidation();
            }
        }
    }
    ResetPrimaryValidation();
    return true;
}

/**
 * SimCheckValidation
 *
 * After simulating moves and scanning enemy coverage, determine if the current player's king is in check.
 *
 * Behavior:
 *  - Iterates the board and sets Player1.SimChecked or Player2.SimChecked if the king of Turn is vulnerable.
 *
 * Side effects:
 *  - Updates Player*.SimChecked flags used by simulation logic.
 */
void SimCheckValidation()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Cell thisCell = GameBoard[row][col];

            if (thisCell.piece.team != Turn)
            {
                continue;
            }

            if (thisCell.piece.type == PIECE_KING && thisCell.vulnerable)
            {
                Turn == TEAM_WHITE ? (Player1.SimChecked = true) : (Player2.SimChecked = true);
            }
        }
    }
}

/**
 * StalemateValidation
 *
 * Determine whether the current position is a stalemate for the side to move.
 *
 * Behavior:
 *  - Uses CheckmateFlagCheck while ensuring the side is not currently in check.
 *  - If no legal move exists and the side is not in check, sets Player.Stalemate and global Stalemate.
 */

void StalemateValidation()
{
    state.whitePlayer.Stalemate = false;
    state.blackPlayer.Stalemate = false;
    if (Turn == TEAM_WHITE)
    {
        /* code */

        if (!Player1.Checked)
        {
            Player1.Stalemate = CheckmateFlagCheck(TEAM_WHITE);
            if (Player1.Stalemate)
            {
                state.isStalemate = true;
            }
        }
    }
    else if (!Player2.Checked)
    {
        Player2.Stalemate = CheckmateFlagCheck(TEAM_BLACK);
        if (Player2.Stalemate)
        {
            state.isStalemate = true;
        }
    }
}

void ResetsAndValidations()
{
    Turn = (Turn == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE; // Added turns
    ResetVulnerable();                                     // Reset the curent piece that are in danger
    ScanEnemyMoves();                                      // Added vulnerability function checker
    CheckValidation();
    StalemateValidation();
    if (Player1.Checked)
    {
        if (Turn == TEAM_WHITE)
        {
            CheckmateValidation();
        }
    }
    else if (Player2.Checked)
    {
        if (Turn == TEAM_BLACK)
        {
            CheckmateValidation();
        }
    }
}

void PromotePawn(PieceType selectedType)
{
    int row = state.promotionRow;
    int col = state.promotionCol;

    if (row == -1 || col == -1)
        return;

    Team team = GameBoard[row][col].piece.team;

    // 1. Replace pawn with new piece
    LoadPiece(row, col, selectedType, team, GAME_BOARD);

    // 2. Clear the promotion state
    state.isPromoting = false;
    state.promotionRow = -1;
    state.promotionCol = -1;

    // 3. Resume game
    ResetsAndValidations();
}

void PrimaryCastlingValidation()
{
    // We use the GameState flags (whiteKingSide, etc.) which are persistent.
    // This fixes the bug where moving a rook away and back would reset its 'hasMoved' status.

    if (Turn == TEAM_WHITE)
    {
        // Check King is present and not in check
        if (GameBoard[WHITE_BACK_RANK][KING_START_COL].piece.type == PIECE_KING && !Player1.Checked)
        {
            // Queen Side (Long Castling)
            if (state.whiteQueenSide)
            {
                // Must be empty: b1, c1, d1
                if (GameBoard[WHITE_BACK_RANK][1].piece.type == PIECE_NONE && /*This is the b1 square but We don't have macros that match it*/
                    GameBoard[WHITE_BACK_RANK][CASTLE_QS_KING_COL].piece.type == PIECE_NONE &&
                    GameBoard[WHITE_BACK_RANK][CASTLE_QS_ROOK_COL].piece.type == PIECE_NONE)
                {
                    // King moves from e1 to c1. Passes d1.
                    if (!GameBoard[WHITE_BACK_RANK][CASTLE_QS_KING_COL].vulnerable &&
                        !GameBoard[WHITE_BACK_RANK][CASTLE_QS_ROOK_COL].vulnerable)
                    {
                        GameBoard[WHITE_BACK_RANK][CASTLE_QS_KING_COL].primaryValid = true;
                    }
                }
            }

            // King Side (Short Castling)
            if (state.whiteKingSide)
            {
                // Must be empty: f1, g1
                if (GameBoard[WHITE_BACK_RANK][CASTLE_KS_ROOK_COL].piece.type == PIECE_NONE &&
                    GameBoard[WHITE_BACK_RANK][CASTLE_KS_KING_COL].piece.type == PIECE_NONE)
                {
                    // King passes through f1, lands on g1.
                    if (!GameBoard[WHITE_BACK_RANK][CASTLE_KS_ROOK_COL].vulnerable &&
                        !GameBoard[WHITE_BACK_RANK][CASTLE_KS_KING_COL].vulnerable)
                    {
                        GameBoard[WHITE_BACK_RANK][CASTLE_KS_KING_COL].primaryValid = true;
                    }
                }
            }
        }
    }
    else // BLACK TEAM
    {
        if (GameBoard[BLACK_BACK_RANK][KING_START_COL].piece.type == PIECE_KING && !Player2.Checked)
        {
            // Queen Side
            if (state.blackQueenSide)
            {
                // Must be empty: b8, c8, d8
                if (GameBoard[BLACK_BACK_RANK][1].piece.type == PIECE_NONE &&
                    GameBoard[BLACK_BACK_RANK][CASTLE_QS_KING_COL].piece.type == PIECE_NONE &&
                    GameBoard[BLACK_BACK_RANK][CASTLE_QS_ROOK_COL].piece.type == PIECE_NONE)
                {
                    if (!GameBoard[BLACK_BACK_RANK][CASTLE_QS_KING_COL].vulnerable &&
                        !GameBoard[BLACK_BACK_RANK][CASTLE_QS_ROOK_COL].vulnerable)
                    {
                        GameBoard[BLACK_BACK_RANK][CASTLE_QS_KING_COL].primaryValid = true;
                    }
                }
            }

            // King Side
            if (state.blackKingSide)
            {
                // Must be empty: f8, g8
                if (GameBoard[BLACK_BACK_RANK][CASTLE_KS_ROOK_COL].piece.type == PIECE_NONE &&
                    GameBoard[BLACK_BACK_RANK][CASTLE_KS_KING_COL].piece.type == PIECE_NONE)
                {
                    if (!GameBoard[BLACK_BACK_RANK][CASTLE_KS_ROOK_COL].vulnerable &&
                        !GameBoard[BLACK_BACK_RANK][CASTLE_KS_KING_COL].vulnerable)
                    {
                        GameBoard[BLACK_BACK_RANK][CASTLE_KS_KING_COL].primaryValid = true;
                    }
                }
            }
        }
    }
}
