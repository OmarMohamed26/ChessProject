/**
 * move.c
 *
 * Responsibilities:
 * - Execute piece movement between cells on the GameBoard.
 * - Provide a helper to clear a cell's piece data and release associated texture.
 * - Implement move validation logic (primary geometric checks and final legal checks).
 * - Handle special moves: Castling, En Passant, Promotion.
 * - Manage game history (Undo/Redo) and state updates (Turn, Check, Mate).
 *
 * Notes:
 * - These functions operate directly on the global GameBoard array (declared in main.c).
 * - Textures are managed with raylib's LoadTexture/UnloadTexture APIs; callers must ensure
 *   proper sizing/assignment (LoadPiece is used to place textures on destination squares).
 * - All operations are intended to be called from the main thread.
 */

#include "move.h"
#include "draw.h"
#include "hash.h"
#include "main.h"
#include "raylib.h"
#include "settings.h"
#include "stack.h"
#include <stdbool.h>
#include <stdlib.h>

bool checked = false, flag = false;

// NEW: Temporary storage for the move while waiting for promotion selection
static Move pendingMove;

// NEW: Helper function to play sounds based on move result
static void PlayGameSound(Move move)
{
    if (state.isCheckmate)
    {
        PlaySound(state.sounds.checkMate);
    }
    else if (state.whitePlayer.Checked || state.blackPlayer.Checked)
    {
        PlaySound(state.sounds.check);
    }
    else if (move.pieceCapturedType != PIECE_NONE)
    {
        PlaySound(state.sounds.capture);
    }
    else
    {
        PlaySound(state.sounds.move);
    }
}

/* Add prototypes near the top of the file (below includes) */
static void RaycastRook(int CellX, int CellY, Team team);
static void RaycastBishop(int CellX, int CellY, Team team);
void CheckInsufficientMaterial(void);
Move RecordMove(int initialRow, int initialCol, int finalRow, int finalCol);

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
 * - Updates game clocks (halfmove/fullmove).
 * - Handles special moves (Castling, En Passant, Promotion).
 * - Updates game history (Undo stack, DHA).
 * - Plays appropriate sound effects.
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

    // --- UPDATE CLOCKS ---
    // Fullmove number increments after Black's move
    if (Turn == TEAM_BLACK)
    {
        state.fullMoveNumber++;
    }

    // Halfmove clock resets on Pawn move or Capture
    if (GameBoard[initialRow][initialCol].piece.type == PIECE_PAWN ||
        GameBoard[finalRow][finalCol].piece.type != PIECE_NONE)
    {
        state.halfMoveClock = 0;
        ClearDHA(state.DHA);
    }
    else
    {
        state.halfMoveClock++;
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

    // 1. CAPTURE MOVE DETAILS BEFORE BOARD CHANGES
    // We must do this here because LoadPiece will overwrite the captured piece
    // and SetEmptyCell will clear the source piece.
    Move currentMove = RecordMove(initialRow, initialCol, finalRow, finalCol);

    state.enPassantCol = -1;

    // DeadPiece Handling
    if (Turn == TEAM_WHITE)
    {
        if (GameBoard[finalRow][finalCol].piece.type != PIECE_NONE && GameBoard[finalRow][finalCol].piece.team == TEAM_BLACK)
        {
            // FIX: Prevent array overflow
            if (deadBlackCounter < (BOARD_SIZE * 2))
            {
                LoadPiece(deadBlackCounter++, 1, GameBoard[finalRow][finalCol].piece.type, TEAM_BLACK, DEAD_BLACK_PIECES);
            }
        }
    }
    else
    {
        if (GameBoard[finalRow][finalCol].piece.type != PIECE_NONE && GameBoard[finalRow][finalCol].piece.team == TEAM_WHITE)
        {
            // FIX: Prevent array overflow
            if (deadWhiteCounter < (BOARD_SIZE * 2))
            {
                LoadPiece(deadWhiteCounter++, 1, GameBoard[finalRow][finalCol].piece.type, TEAM_WHITE, DEAD_WHITE_PIECES);
            }
        }
    }

    // --- CHANGED: Castling Logic ---
    // We now detect castling by checking if the King moved 2 squares horizontally.

    PieceType movingPieceType = GameBoard[initialRow][initialCol].piece.type;
    ResetJustMoved();
    GameBoard[finalRow][finalCol].JustMoved = true;

    if (movingPieceType == PIECE_KING && abs(finalCol - initialCol) == 2)
    {
        if (Turn == TEAM_WHITE)
        {
            if (finalRow == WHITE_BACK_RANK && finalCol == CASTLE_KS_KING_COL) // White King Side (g1)
            {
                PushStack(state.undoStack, currentMove);
                PlayGameSound(currentMove);
                ClearStack(state.redoStack);
                state.whiteKingSide = false;
                state.whiteQueenSide = false;
                LoadPiece(WHITE_BACK_RANK, CASTLE_KS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][KING_START_COL]);
                LoadPiece(WHITE_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_KS_COL]);
                ResetsAndValidations();
                return;
            }
            if (finalRow == WHITE_BACK_RANK && finalCol == CASTLE_QS_KING_COL) // White Queen Side (c1)
            {
                PushStack(state.undoStack, currentMove);
                PlayGameSound(currentMove);
                ClearStack(state.redoStack);
                state.whiteKingSide = false;
                state.whiteQueenSide = false;
                LoadPiece(WHITE_BACK_RANK, CASTLE_QS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][KING_START_COL]);
                LoadPiece(WHITE_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_QS_COL]);

                ResetsAndValidations();
                return;
            }
        }
        else if (Turn == TEAM_BLACK)
        {
            if (finalRow == BLACK_BACK_RANK && finalCol == CASTLE_KS_KING_COL) // Black King Side (g8)
            {
                PushStack(state.undoStack, currentMove);
                PlayGameSound(currentMove);
                ClearStack(state.redoStack);
                state.blackKingSide = false;
                state.blackQueenSide = false;
                LoadPiece(BLACK_BACK_RANK, CASTLE_KS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][KING_START_COL]);
                LoadPiece(BLACK_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_KS_COL]);
                ResetsAndValidations();
                return;
            }
            if (finalRow == BLACK_BACK_RANK && finalCol == CASTLE_QS_KING_COL) // Black Queen Side (c8)
            {
                PushStack(state.undoStack, currentMove);
                PlayGameSound(currentMove);
                ClearStack(state.redoStack);
                state.blackKingSide = false;
                state.blackQueenSide = false;
                LoadPiece(BLACK_BACK_RANK, CASTLE_QS_KING_COL, PIECE_KING, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][KING_START_COL]);
                LoadPiece(BLACK_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_QS_COL]);
                ResetsAndValidations();
                return;
            }
        }
    }

    if (state.enPassantCol != -1)
    {
        if (Turn == TEAM_WHITE)
        {
            if (movingPieceType == PIECE_PAWN && finalRow == 2 && finalCol == initialCol - 1)
            {
                LoadPiece(finalRow, finalCol, PIECE_PAWN, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[initialRow][initialCol]);
                SetEmptyCell(&GameBoard[initialRow][initialCol - 1]);
            }
            if (movingPieceType == PIECE_PAWN && finalRow == 2 && finalCol == initialCol + 1)
            {
                LoadPiece(finalRow, finalCol, PIECE_PAWN, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[initialRow][initialCol]);
                SetEmptyCell(&GameBoard[initialRow][initialCol + 1]);
            }
        }
        else
        {
            if (movingPieceType == PIECE_PAWN && finalRow == 5 && finalCol == initialCol - 1)
            {
                LoadPiece(finalRow, finalCol, PIECE_PAWN, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[initialRow][initialCol]);
                SetEmptyCell(&GameBoard[initialRow][initialCol - 1]);
            }
            if (movingPieceType == PIECE_PAWN && finalRow == 5 && finalCol == initialCol + 1)
            {
                LoadPiece(finalRow, finalCol, PIECE_PAWN, Turn, GAME_BOARD);
                SetEmptyCell(&GameBoard[initialRow][initialCol]);
                SetEmptyCell(&GameBoard[initialRow][initialCol + 1]);
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
    // Update implementing PawnMovedTwoFlag
    if (GameBoard[initialRow][initialCol].piece.type == PIECE_PAWN && abs(finalRow - initialRow) == 2)
    {
        GameBoard[finalRow][finalCol].PawnMovedTwo = true;
    }

    // 1. Move the piece
    LoadPiece(finalRow, finalCol, GameBoard[initialRow][initialCol].piece.type, GameBoard[initialRow][initialCol].piece.team, GAME_BOARD);
    GameBoard[finalRow][finalCol].piece.hasMoved = 1;
    SetEmptyCell(&GameBoard[initialRow][initialCol]);

    // FIX: Execute En Passant Capture
    // We must do this explicitly here because we reset state.enPassantCol to -1 earlier.
    if (currentMove.wasEnPassant)
    {
        // The captured pawn is at [initialRow][finalCol]
        SetEmptyCell(&GameBoard[initialRow][finalCol]);
    }

    // --- NEW: Check for Promotion ---
    bool isPromoting = false;

    if (GameBoard[finalRow][finalCol].piece.type == PIECE_PAWN)
    {
        // White reaches row 0, Black reaches row 7 (assuming 0 is top)
        if ((GameBoard[finalRow][finalCol].piece.team == TEAM_WHITE && finalRow == 0) ||
            (GameBoard[finalRow][finalCol].piece.team == TEAM_BLACK && finalRow == BOARD_SIZE - 1))
        {
            isPromoting = true;
        }
    }

    if (isPromoting)
    {
        state.isPromoting = true;
        state.promotionRow = finalRow;
        state.promotionCol = finalCol;

        // SAVE THE MOVE FOR LATER
        pendingMove = currentMove;

        // RETURN EARLY: Pause the game, wait for input
        return;
    }

    state.promotionType = PIECE_NONE;
    PushStack(state.undoStack, currentMove);
    ClearStack(state.redoStack);

    ResetsAndValidations();

    // --- HISTORY HANDLING ---
    Hash currentHash = CurrentGameStateHash();

    // 1. Check for Draw (only if reversible move)
    if (state.halfMoveClock > 0)
    {
        if (IsRepeated3times(state.DHA, currentHash))
        {
            state.isRepeated3times = true;
        }
    }

    // 2. Record the move
    PushDHA(state.DHA, currentHash);

    // NEW: Play sound for normal moves
    PlayGameSound(currentMove);
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

/**
 * ResetMovedStatus
 *
 * Resets the 'hasMoved' flag for all pieces on the board.
 * Used when loading a new game or resetting state.
 */
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
 * ResetJustMoved
 *
 * Resets the 'JustMoved' flag for all pieces.
 * This flag is used to track the last moved piece for En Passant logic.
 */
void ResetJustMoved()
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            GameBoard[row][col].JustMoved = false;
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
 *  - selected : true if the piece is currently selected (triggers special checks like castling).
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
        if (selected)
        {
            PrimaryEnpassantValidation(CellX, CellY);
        }

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

        if (CellY + 1 < BOARD_SIZE)
        {
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
        }

        if (CellY - 1 >= 0)
        {
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

        if (CellY - 1 >= 0)
        {
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
        }

        if (CellY + 1 < BOARD_SIZE)
        {
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
        if (Turn == TEAM_WHITE)
        {
            // FIX: Check CellY - 1 >= 0
            if (piece1 == PIECE_PAWN && CellY - 1 >= 0 &&
                GameBoard[CellX - 1][CellY - 1].isvalid &&
                GameBoard[CellX - 1][CellY - 1].piece.type == PIECE_NONE)
            {
                state.enPassantCol = CellY - 1;
            }
            // FIX: Check CellY + 1 < BOARD_SIZE
            else if (piece1 == PIECE_PAWN && CellY + 1 < BOARD_SIZE &&
                     GameBoard[CellX - 1][CellY + 1].isvalid &&
                     GameBoard[CellX - 1][CellY + 1].piece.type == PIECE_NONE)
            {
                state.enPassantCol = CellY + 1;
            }
        }
        else
        {
            // FIX: Check CellY - 1 >= 0
            if (piece1 == PIECE_PAWN && CellY - 1 >= 0 &&
                GameBoard[CellX + 1][CellY - 1].isvalid &&
                GameBoard[CellX + 1][CellY - 1].piece.type == PIECE_NONE)
            {
                state.enPassantCol = CellY - 1;
            }
            // FIX: Check CellY + 1 < BOARD_SIZE
            else if (piece1 == PIECE_PAWN && CellY + 1 < BOARD_SIZE &&
                     GameBoard[CellX + 1][CellY + 1].isvalid &&
                     GameBoard[CellX + 1][CellY + 1].piece.type == PIECE_NONE)
            {
                state.enPassantCol = CellY + 1;
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

/**
 * ResetsAndValidations
 *
 * The central update routine called after a move is made or undone.
 *
 * Responsibilities:
 * 1. Flips the turn (White <-> Black).
 * 2. Clears previous validation flags (isvalid, primaryValid).
 * 3. Re-calculates board state:
 *    - Resets vulnerability map.
 *    - Scans enemy moves to update threats.
 *    - Checks if the current player is in Check.
 *    - Checks for Stalemate or Checkmate.
 *    - Checks for Insufficient Material.
 */
void ResetsAndValidations()
{
    Turn = (Turn == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE; // Added turns

    // --- FIX: Clear validation flags ---
    // This ensures that any "valid moves" calculated for the previous state
    // (e.g. a selected piece) are wiped out when the state changes via Undo/Redo.
    ResetValidation();
    ResetPrimaryValidation();
    // -----------------------------------

    ResetVulnerable(); // Reset the curent piece that are in danger
    ScanEnemyMoves();  // Added vulnerability function checker
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

    CheckInsufficientMaterial();
}

/**
 * PromotePawn
 *
 * Finalizes a pending pawn promotion.
 *
 * Parameters:
 *  - selectedType: The piece type chosen by the user (Queen, Rook, Bishop, Knight).
 *
 * Behavior:
 *  - Replaces the pawn at the promotion square with the new piece.
 *  - Records the move in history with the correct promotion type.
 *  - Resumes the game loop (calls ResetsAndValidations).
 *  - Plays the promotion sound.
 */
void PromotePawn(PieceType selectedType)
{
    int row = state.promotionRow;
    int col = state.promotionCol;

    if (row == -1 || col == -1)
    {
        return;
    }

    Team team = GameBoard[row][col].piece.team;

    // 1. Replace pawn with new piece
    LoadPiece(row, col, selectedType, team, GAME_BOARD);

    // 2. Clear the promotion state
    state.isPromoting = false;
    state.promotionRow = -1;
    state.promotionCol = -1;

    state.promotionType = selectedType;

    // 3. FINALIZE RECORDING THE MOVE
    pendingMove.promotionType = selectedType;
    PushStack(state.undoStack, pendingMove);
    ClearStack(state.redoStack);

    // 4. Resume game
    ResetsAndValidations();

    // NEW: Play sound for promotion moves
    PlayGameSound(pendingMove);
}

/**
 * PrimaryCastlingValidation
 *
 * Checks if castling moves are geometrically possible (path clear, not attacked).
 *
 * Behavior:
 *  - Checks global castling rights flags (whiteKingSide, etc.).
 *  - Verifies that squares between King and Rook are empty.
 *  - Verifies that the King does not pass through or land on an attacked square.
 *  - Sets primaryValid on the King's destination square if valid.
 */
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

/**
 * PrimaryEnpassantValidation
 *
 * Checks if En Passant capture is possible.
 *
 * Parameters:
 *  - row, col: Coordinates of the pawn attempting to capture.
 *
 * Behavior:
 *  - Checks if the pawn is on the correct rank (3 for White, 4 for Black).
 *  - Checks adjacent squares for an enemy pawn that just moved two squares.
 *  - Sets primaryValid on the destination square (behind the captured pawn).
 */
void PrimaryEnpassantValidation(int row, int col)
{
    if (Turn == TEAM_WHITE)
    {
        if (GameBoard[row][col].piece.type == PIECE_PAWN && row == 3)
        {
            // Check Right: Ensure col + 1 is within bounds
            if (col + 1 < BOARD_SIZE &&
                GameBoard[row][col + 1].JustMoved &&
                GameBoard[row][col + 1].PawnMovedTwo)
            {
                GameBoard[row - 1][col + 1].primaryValid = true;
            }

            // Check Left: Ensure col - 1 is within bounds
            if (col - 1 >= 0 &&
                GameBoard[row][col - 1].JustMoved &&
                GameBoard[row][col - 1].PawnMovedTwo)
            {
                GameBoard[row - 1][col - 1].primaryValid = true;
            }
        }
    }
    else
    {
        if (GameBoard[row][col].piece.type == PIECE_PAWN && row == 4)
        {
            // Check Right: Ensure col + 1 is within bounds
            if (col + 1 < BOARD_SIZE &&
                GameBoard[row][col + 1].JustMoved &&
                GameBoard[row][col + 1].PawnMovedTwo)
            {
                GameBoard[row + 1][col + 1].primaryValid = true;
            }

            // Check Left: Ensure col - 1 is within bounds
            if (col - 1 >= 0 &&
                GameBoard[row][col - 1].JustMoved &&
                GameBoard[row][col - 1].PawnMovedTwo)
            {
                GameBoard[row + 1][col - 1].primaryValid = true;
            }
        }
    }
}

/**
 * CheckInsufficientMaterial
 *
 * Checks if the remaining pieces on the board are insufficient to force a checkmate.
 *
 * Scenarios detected:
 * 1. King vs King.
 * 2. King + Minor Piece (Bishop/Knight) vs King.
 * 3. King + Bishop vs King + Bishop (where both bishops are on the same color square).
 *
 * Side effects:
 *  - Sets state.isInsufficientMaterial to true if draw condition is met.
 */
void CheckInsufficientMaterial(void)
{
    int whiteMinorPieces = 0;
    int blackMinorPieces = 0;

    // Track bishops specifically for the K+B vs K+B scenario
    int whiteBishops = 0;
    int blackBishops = 0;
    int whiteBishopSquareColor = -1; // 0 for light, 1 for dark
    int blackBishopSquareColor = -1;

    // Scan the board
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            PieceType type = GameBoard[i][j].piece.type;
            Team team = GameBoard[i][j].piece.team;

            if (type == PIECE_NONE)
            {
                continue;
            }

            // If there is a Queen, Rook, or Pawn, checkmate is possible.
            if (type == PIECE_QUEEN || type == PIECE_ROOK || type == PIECE_PAWN)
            {
                state.isInsufficientMaterial = false;
                return;
            }

            if (type == PIECE_KNIGHT || type == PIECE_BISHOP)
            {
                if (team == TEAM_WHITE)
                {
                    whiteMinorPieces++;
                    if (type == PIECE_BISHOP)
                    {
                        whiteBishops++;
                        // Determine square color: (row + col) % 2
                        whiteBishopSquareColor = (i + j) % 2;
                    }
                }
                else
                {
                    blackMinorPieces++;
                    if (type == PIECE_BISHOP)
                    {
                        blackBishops++;
                        blackBishopSquareColor = (i + j) % 2;
                    }
                }
            }
        }
    }

    // SCENARIO 1: King vs King (No minor pieces)
    if (whiteMinorPieces == 0 && blackMinorPieces == 0)
    {
        state.isInsufficientMaterial = true;
        return;
    }

    // SCENARIO 2: King + Minor vs King (One side has 1 minor, other has 0)
    if ((whiteMinorPieces == 1 && blackMinorPieces == 0) ||
        (whiteMinorPieces == 0 && blackMinorPieces == 1))
    {
        state.isInsufficientMaterial = true;
        return;
    }

    // SCENARIO 3: King + Bishop vs King + Bishop (Same color)
    // Conditions:
    // 1. Each side has exactly 1 minor piece.
    // 2. That minor piece is a Bishop.
    // 3. Both bishops are on the same square color.
    if (whiteMinorPieces == 1 && blackMinorPieces == 1 &&
        whiteBishops == 1 && blackBishops == 1) // <--- FIX: Added "== 1"
    {
        if (whiteBishopSquareColor == blackBishopSquareColor)
        {
            state.isInsufficientMaterial = true;
            return;
        }
    }
    state.isInsufficientMaterial = false;
}

/**
 * RecordMove
 *
 * Creates a Move structure capturing all details of the current move.
 * Used for history tracking (Undo/Redo).
 *
 * Parameters:
 *  - initialRow, initialCol: Source coordinates.
 *  - finalRow, finalCol: Destination coordinates.
 *
 * Returns:
 *  - A populated Move structure.
 */
Move RecordMove(int initialRow, int initialCol, int finalRow, int finalCol)
{
    Move move;
    move.initialRow = initialRow;
    move.initialCol = initialCol;
    move.finalRow = finalRow;
    move.finalCol = finalCol;

    move.pieceMovedType = state.board[initialRow][initialCol].piece.type;
    move.pieceMovedTeam = state.board[initialRow][initialCol].piece.team;

    move.pieceCapturedType = state.board[finalRow][finalCol].piece.type;

    // The promotion piece type will be recorded in another place in the program
    move.promotionType = PIECE_NONE;

    if (move.pieceMovedType == PIECE_PAWN &&
        initialCol != finalCol &&
        GameBoard[finalRow][finalCol].piece.type == PIECE_NONE)
    {
        move.wasEnPassant = true;
        move.pieceCapturedType = PIECE_PAWN;
    }
    else
    {
        move.wasEnPassant = false;
    }

    move.previousEnPassantCol = state.enPassantCol;

    if (state.board[initialRow][initialCol].piece.type == PIECE_KING && abs(finalCol - initialCol) == 2)
    {
        move.wasCastling = true;
    }
    else
    {
        move.wasCastling = false;
    }

    move.whiteKingSide = state.whiteKingSide;
    move.whiteQueenSide = state.whiteQueenSide;
    move.blackKingSide = state.blackKingSide;
    move.blackQueenSide = state.blackQueenSide;

    move.halfMove = state.halfMoveClock;

    return move;
}

/**
 * UndoMove
 *
 * Reverts the last move made in the game.
 *
 * Behavior:
 * - Pops the last move from the Undo stack.
 * - Restores board state (pieces, captured pieces, castling rights, en passant).
 * - Pushes the undone move to the Redo stack.
 * - Updates game history (DHA) and visuals.
 * - Plays undo sound.
 */
void UndoMove(void)
{
    Move move;

    if (!PopStack(state.undoStack, &move))
    {
        // The stack is empty so you can't undo
        return;
    }

    // 1. Restore Global State Flags
    // Note: We do NOT flip the Turn here manually, because ResetsAndValidations()
    // at the end will flip it for us.

    state.halfMoveClock = move.halfMove;

    // If we are undoing a move that transitioned to Black, we decrement full moves
    if (Turn == TEAM_BLACK)
    {
        state.fullMoveNumber--;
    }

    state.enPassantCol = move.previousEnPassantCol;

    state.whiteKingSide = move.whiteKingSide;
    state.whiteQueenSide = move.whiteQueenSide;
    state.blackKingSide = move.blackKingSide;
    state.blackQueenSide = move.blackQueenSide;

    // Clear flags that might have been set by the "future" state
    state.isStalemate = false;
    state.isRepeated3times = false;
    state.isInsufficientMaterial = false;
    Checkmate = false;
    Player1.Checkmated = false;
    Player2.Checkmated = false;

    // 2. Move the piece back
    // This handles Un-Promotion automatically because move.pieceMovedType
    // stores the original piece (PAWN), not the promoted one.
    LoadPiece(move.initialRow, move.initialCol, move.pieceMovedType, move.pieceMovedTeam, GAME_BOARD);

    // Clear the destination square (unless it was a capture, which we handle next)
    SetEmptyCell(&GameBoard[move.finalRow][move.finalCol]);

    // 3. Restore Captured Piece
    if (move.pieceCapturedType != PIECE_NONE)
    {
        Team capturedTeam = (move.pieceMovedTeam == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;

        if (move.wasEnPassant)
        {
            // En Passant Special Case:
            // The captured pawn was NOT at the destination (finalRow, finalCol).
            // ! It was at (initialRow, finalCol).
            LoadPiece(move.initialRow, move.finalCol, move.pieceCapturedType, capturedTeam, GAME_BOARD);
        }
        else
        {
            // Normal Capture:
            // Put the captured piece back at the destination square.
            LoadPiece(move.finalRow, move.finalCol, move.pieceCapturedType, capturedTeam, GAME_BOARD);
        }

        if (capturedTeam == TEAM_WHITE && deadWhiteCounter > 0)
        {
            deadWhiteCounter--;
        }
        if (capturedTeam == TEAM_BLACK && deadBlackCounter > 0)
        {
            deadBlackCounter--;
        }
    }

    // 4. Restore Castling (Move the Rook back) the king was handled by section 2 Move the piece back
    if (move.wasCastling)
    {
        // White King Side
        if (move.finalRow == WHITE_BACK_RANK && move.finalCol == CASTLE_KS_KING_COL)
        {
            // Move Rook from f1 back to h1
            LoadPiece(WHITE_BACK_RANK, ROOK_KS_COL, PIECE_ROOK, TEAM_WHITE, GAME_BOARD);
            SetEmptyCell(&GameBoard[WHITE_BACK_RANK][CASTLE_KS_ROOK_COL]);
        }
        // White Queen Side
        else if (move.finalRow == WHITE_BACK_RANK && move.finalCol == CASTLE_QS_KING_COL)
        {
            // Move Rook from d1 back to a1
            LoadPiece(WHITE_BACK_RANK, ROOK_QS_COL, PIECE_ROOK, TEAM_WHITE, GAME_BOARD);
            SetEmptyCell(&GameBoard[WHITE_BACK_RANK][CASTLE_QS_ROOK_COL]);
        }
        // Black King Side
        else if (move.finalRow == BLACK_BACK_RANK && move.finalCol == CASTLE_KS_KING_COL)
        {
            // Move Rook from f8 back to h8
            LoadPiece(BLACK_BACK_RANK, ROOK_KS_COL, PIECE_ROOK, TEAM_BLACK, GAME_BOARD);
            SetEmptyCell(&GameBoard[BLACK_BACK_RANK][CASTLE_KS_ROOK_COL]);
        }
        // Black Queen Side
        else if (move.finalRow == BLACK_BACK_RANK && move.finalCol == CASTLE_QS_KING_COL)
        {
            // Move Rook from d8 back to a8
            LoadPiece(BLACK_BACK_RANK, ROOK_QS_COL, PIECE_ROOK, TEAM_BLACK, GAME_BOARD);
            SetEmptyCell(&GameBoard[BLACK_BACK_RANK][CASTLE_QS_ROOK_COL]);
        }
    }

    PopDHA(state.DHA); // It's safe to call this function even if the DHA is empty. anyways an empty DHA shouldn't be reachable here it should be caught at the beginning of the code if PopStack fails.

    // 6. Push the move we have just undone to the Redo stack

    PushStack(state.redoStack, move);

    // When we undo, we restore the state where an En Passant capture might be possible.
    // We must mark the specific pawn as having "JustMoved" and "PawnMovedTwo".
    if (state.enPassantCol != -1) // It was EnPassant.
    {
        // If we undid a White move, it's White's turn, so the target is a Black pawn at row 3.
        // If we undid a Black move, it's Black's turn, so the target is a White pawn at row 4.
        int row = (move.pieceMovedTeam == TEAM_WHITE) ? 3 : 4;

        // Ensure we are accessing valid bounds
        if (row >= 0 && row < BOARD_SIZE && state.enPassantCol >= 0 && state.enPassantCol < BOARD_SIZE)
        {
            GameBoard[row][state.enPassantCol].JustMoved = true;
            GameBoard[row][state.enPassantCol].PawnMovedTwo = true;
        }
    }

    // 7. Recalculate Valid Moves for the restored state
    // This function flips the turn, scans enemy moves, and checks for check/mate.
    ResetsAndValidations();

    // 8. Restore SmartBorders (Visuals)
    // We need to highlight the move that is NOW at the top of the stack (the one before the undo).
    if (state.undoStack->size > 0)
    {
        // Peek at the previous move without popping it
        Move previousMove = state.undoStack->data[state.undoStack->size - 1];

        // Update the visual border to point to that move's destination
        UpdateLastMoveHighlight(previousMove.finalRow, previousMove.finalCol);
    }
    else
    {
        // No moves left in history, clear the border
        UpdateLastMoveHighlight(-1, -1);
    }

    ResetSelectedPiece();

    // NEW: Play move sound on Undo
    PlayGameSound(move);
}

/**
 * RedoMove
 *
 * Re-applies a move that was previously undone.
 *
 * Behavior:
 * - Pops the move from the Redo stack.
 * - Re-executes the move on the board (including captures, castling, promotion).
 * - Pushes the move back to the Undo stack.
 * - Updates game history and visuals.
 * - Plays move sound.
 */
void RedoMove(void)
{
    Move move;

    if (!PopStack(state.redoStack, &move))
    {
        return;
    }

    // 1. Push to Undo Stack
    PushStack(state.undoStack, move);

    // 2. Update Clocks
    if (move.pieceMovedTeam == TEAM_BLACK)
    {
        state.fullMoveNumber++;
    }

    if (move.pieceMovedType == PIECE_PAWN || move.pieceCapturedType != PIECE_NONE)
    {
        state.halfMoveClock = 0;
        ClearDHA(state.DHA);
    }
    else
    {
        state.halfMoveClock++;
    }

    // 3. Execute Move on Board
    // If promotionType is set, use it. Otherwise use the original piece type.
    PieceType typeToPlace = (move.promotionType != PIECE_NONE) ? move.promotionType : move.pieceMovedType;

    LoadPiece(move.finalRow, move.finalCol, typeToPlace, move.pieceMovedTeam, GAME_BOARD);
    GameBoard[move.finalRow][move.finalCol].piece.hasMoved = 1;
    SetEmptyCell(&GameBoard[move.initialRow][move.initialCol]);

    // 4. Handle Special Moves

    // En Passant Capture
    if (move.wasEnPassant)
    {
        SetEmptyCell(&GameBoard[move.initialRow][move.finalCol]); // ! see a note in this weird thing in UndoMove
    }

    // Castling (Move the Rook)  the king was handled by section 3
    if (move.wasCastling)
    {
        if (move.pieceMovedTeam == TEAM_WHITE)
        {
            if (move.finalCol == CASTLE_KS_KING_COL) // KS
            {
                LoadPiece(WHITE_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, TEAM_WHITE, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_KS_COL]);
            }
            else // QS
            {
                LoadPiece(WHITE_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, TEAM_WHITE, GAME_BOARD);
                SetEmptyCell(&GameBoard[WHITE_BACK_RANK][ROOK_QS_COL]);
            }
        }
        else
        {
            if (move.finalCol == CASTLE_KS_KING_COL) // KS
            {
                LoadPiece(BLACK_BACK_RANK, CASTLE_KS_ROOK_COL, PIECE_ROOK, TEAM_BLACK, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_KS_COL]);
            }
            else // QS
            {
                LoadPiece(BLACK_BACK_RANK, CASTLE_QS_ROOK_COL, PIECE_ROOK, TEAM_BLACK, GAME_BOARD);
                SetEmptyCell(&GameBoard[BLACK_BACK_RANK][ROOK_QS_COL]);
            }
        }
    }

    // 5. Update Castling Rights
    // If King moved
    if (move.pieceMovedType == PIECE_KING)
    {
        if (move.pieceMovedTeam == TEAM_WHITE)
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
    // If Rook moved
    if (move.pieceMovedType == PIECE_ROOK)
    {
        if (move.pieceMovedTeam == TEAM_WHITE)
        {
            if (move.initialRow == WHITE_BACK_RANK && move.initialCol == ROOK_QS_COL)
            {
                state.whiteQueenSide = false;
            }
            if (move.initialRow == WHITE_BACK_RANK && move.initialCol == ROOK_KS_COL)
            {
                state.whiteKingSide = false;
            }
        }
        else
        {
            if (move.initialRow == BLACK_BACK_RANK && move.initialCol == ROOK_QS_COL)
            {
                state.blackQueenSide = false;
            }
            if (move.initialRow == BLACK_BACK_RANK && move.initialCol == ROOK_KS_COL)
            {
                state.blackKingSide = false;
            }
        }
    }
    // If Rook captured
    if (move.pieceCapturedType == PIECE_ROOK)
    {
        if (move.pieceMovedTeam == TEAM_WHITE) // White captured Black Rook
        {
            if (move.finalRow == BLACK_BACK_RANK && move.finalCol == ROOK_QS_COL)
            {
                state.blackQueenSide = false;
            }
            if (move.finalRow == BLACK_BACK_RANK && move.finalCol == ROOK_KS_COL)
            {
                state.blackKingSide = false;
            }
        }
        else // Black captured White Rook
        {
            if (move.finalRow == WHITE_BACK_RANK && move.finalCol == ROOK_QS_COL)
            {
                state.whiteQueenSide = false;
            }
            if (move.finalRow == WHITE_BACK_RANK && move.finalCol == ROOK_KS_COL)
            {
                state.whiteKingSide = false;
            }
        }
    }

    // 6. Update En Passant Flag
    state.enPassantCol = -1;
    if (move.pieceMovedType == PIECE_PAWN && abs(move.finalRow - move.initialRow) == 2)
    {
        state.enPassantCol = move.finalCol;
        GameBoard[move.finalRow][move.finalCol].PawnMovedTwo = true;
    }

    // 7. Dead Pieces
    if (move.pieceCapturedType != PIECE_NONE)
    {
        if (move.pieceMovedTeam == TEAM_WHITE)
        {
            if (deadBlackCounter < (BOARD_SIZE * 2))
            {
                LoadPiece(deadBlackCounter++, 1, move.pieceCapturedType, TEAM_BLACK, DEAD_BLACK_PIECES);
            }
        }
        else
        {
            if (deadWhiteCounter < (BOARD_SIZE * 2))
            {
                LoadPiece(deadWhiteCounter++, 1, move.pieceCapturedType, TEAM_WHITE, DEAD_WHITE_PIECES);
            }
        }
    }

    // 8. History & Validation
    ResetsAndValidations();

    Hash currentHash = CurrentGameStateHash();
    if (state.halfMoveClock > 0)
    {
        if (IsRepeated3times(state.DHA, currentHash))
        {
            state.isRepeated3times = true;
        }
    }

    PushDHA(state.DHA, currentHash);

    // 9. Visuals
    UpdateLastMoveHighlight(move.finalRow, move.finalCol);

    // 10. Play Sounds

    PlayGameSound(move);
}
