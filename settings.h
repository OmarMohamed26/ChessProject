#ifndef SETTINGS_H
#define SETTINGS_H
/*This file has some constants not all of them*/

// These settings are at build time you can't change them after the game has been built
enum settings
{
     // draw.c settings
     FONT_SQUARE_LENGTH_COEFFICIENT = 4, // the bigger the number the smaller the font
     FONT_MIN = 10,
     FONT_GAP_COEFFICIENT = 4,               // the bigger the coeff. the smaller the gap is
     CELL_BORDER_THICKNESS_COEFFICIENT = 15, // the bigger the number the smaller the thickness
     MAX_PIECE_NAME_BUFFER_SIZE = 63,        // the whole path counts not just the file name
     MAX_FEN_BUFFER_SIZE = 127,
     MAX_HALF_FULL_MOVE_DIGITS = 10,
     HIGHLIGHT_COLOR_AMOUNT = 30,              // the bigger the number the more the highlight effect is visible
     MAX_VALID_COLOR = 255,                    // don't change this it's not customizable this is how colors work
     VALID_MOVE_CIRCLE_SQUARE_COEFFICIENT = 3, // the bigger the number the smaller the circle
     FULL_VALID_MOVE_RADIUS = 100,
     INNER_VALID_MOVE_RADIUS = 80, // this is out of FULL_VALID_MOVE_RADIUS
     OUTER_VALID_MOVE_RADIUS = 90, // this is out of FULL_VALID_MOVE_RADIUS
     FULL_CIRCLE_ANGLE = 360,      // this shouldn't be changed at all

     // move.c
     // Castling
     WHITE_BACK_RANK = 7,
     BLACK_BACK_RANK = 0,

     // source columns
     KING_START_COL = 4,
     ROOK_QS_COL = 0, // Queen Side Rook Start (a-file)
     ROOK_KS_COL = 7, // Queen Side Rook Start (a-file)

     // Destination columns for King after castling
     CASTLE_KS_KING_COL = 6, // g-file
     CASTLE_QS_KING_COL = 2, // c-file

     // Destination columns for Rook after castling
     CASTLE_KS_ROOK_COL = 5, // f-file
     CASTLE_QS_ROOK_COL = 3, // d-file

     // main.c
     START_SCREEN_WIDTH = 1280,
     START_SCREEN_HEIGHT = 720,
     MIN_SCREEN_WIDTH = 480,
     FPS = 60,

};

#define BOARD_SIZE 8
#endif