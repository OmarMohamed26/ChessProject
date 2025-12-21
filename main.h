/*
 * main.h
 *
 * Core game types used across the project.
 * Keep this header small and self-contained so other modules can include it.

 VERY IMPORTANT NOTE:
 ! I added an extra bit for the enums because wether its signed or unsigned is implementation defined an extra bit will make us guarantee that it works as intended

 */

#ifndef MAIN_H
#define MAIN_H

#include "raylib.h"

/* PieceType
 * - PIECE_NONE == 0 so zero-initialized memory means "empty cell".
 */
typedef enum
{
    PIECE_NONE = 0, /* empty square */
    PIECE_KING,
    PIECE_QUEEN,
    PIECE_ROOK,
    PIECE_BISHOP,
    PIECE_KNIGHT,
    PIECE_PAWN
} PieceType;

/* Team (side/color) */
typedef enum
{
    TEAM_WHITE = 0,
    TEAM_BLACK
} Team;

/*We need 4 bits to store castle rights in game state think of it in binary*/
// 1111 all rights are reserved
// 1001 white can castle king side and black can castle queen side
typedef enum
{
    WHITE_KING_SIDE = 1,
    WHITE_QUEEN_SIDE = 2,
    BLACK_KING_SIDE = 4,
    BLACK_QUEEN_SIDE = 8

} CastleRights;

/* Piece
 * Represents a single chess piece and its small state.
 *
 * Notes:
 * - `type` and `team` identify the piece.
 * - `hasMoved` / `enPassant` are small flags (0/1).
 * - `spriteId` is an optional index you can use for atlases.
 * - `texture` is a GPU texture (Texture2D). The module that replaces a texture
 *   is responsible for UnloadTexture() on the old texture to avoid GPU leaks.
 */
typedef struct Piece
{
    Texture2D texture;  /* put large/aligned field first to avoid padding gaps */
    PieceType type : 4; // I added an extra bit for the enums because wether its signed or unsigned is implementation defined an extra bit will make us guarantee that it works as intended
    Team team : 2;
    unsigned char hasMoved : 1; /* single byte; now sits after other 4-byte fields */
    // Saved 8 bytes with these bitfields and it will also help us debug errors
} Piece;

/* Cell
 * Single board square with logical coords, render position, and stored piece.
 *
 * - row/col: board indices (0..7)
 * - pos: top-left pixel position for drawing (use DrawTexturePro to scale)
 * - piece: content of the square
 */
typedef struct Cell
{
    Piece piece;                   /* piece occupying the cell (PIECE_NONE if empty) */
    Vector2 pos;                   /* pixel position for rendering (top-left) */
    unsigned int row : 4, col : 4; /* board coordinates (0..7) */
    bool primaryValid : 1;         // This is a primary validation geometrically
    bool isvalid : 1;              // Final validation of moves FINAL_VALIDATION
    bool selected : 1;             // will also need this
    bool vulnerable : 1;           // what pieces are under attack on my team this will help in EASY_MODE
    // Saved 8 bytes with these bitfields and it will also help us debug errors

} Cell;

typedef struct Player
{
    Team team : 2;
    bool Checked : 1;
    bool Checkmated : 1;
    bool SimChecked : 1;
    bool Stalemate : 1;
    // Saved 1 byte with these bitfields and it will also help us debug errors

} Player;

typedef struct __attribute__((packed))
{
    // Done a very good job packing all this info in 6 bytes actually 45 bits

    // Squares
    unsigned int initialRow : 4, initialCol : 4;
    unsigned int finalRow : 4, finalCol : 4;

    // Piece info
    PieceType pieceMovedType : 4;
    Team pieceMovedTeam : 4;
    PieceType pieceCapturedType : 4; // this should be PIECE_NONE if I didn't capture any piece
    // We don't need pieceCapturedTeam because it must be the opposite of PieceMovedTeam

    // Promotion
    PieceType promotionType : 4; // If my piece was pawn and it got promoted what is its new type so that I can undo it **** this should be PIECE_NONE if the pawn didn't promote in this move

    // En-Passant
    unsigned int wasEnPassant : 1;
    unsigned int previousEnPassantCol : 3;
    // This is why we don't need the previousEnPassantRow
    // if (sideToMove == WHITE)
    //     enPassantRow = 2;
    // else
    //     enPassantRow = 5;

    // Castling
    unsigned int wasCastling : 1;
    CastleRights castleRights : 5;

    // Draw
    unsigned int halfMove : 7;

} Move;

extern int pointer;
extern Team Turn;
extern bool Checkmate, Stalemate;
#endif /* MAIN_H */