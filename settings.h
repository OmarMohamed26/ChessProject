/*This file has some constants not all of them*/

// These settings are at build time you can't change them after the game has been built
enum settings
{
     BOARD_SIZE = 8, // this constant shouldn't be changed at all because the game is built with 8x8 assumptions

     // draw.c settings
     FONT_SQUARE_LENGTH_COEFFICIENT = 4, // the bigger the number the smaller the font
     FONT_MIN = 10,
     FONT_GAP_COEFFICIENT = 4,                 // the bigger the coeff. the smaller the gap is
     CELL_BORDER_THICKNESS_COEFFICIENT = 15,   // the bigger the number the smaller the thickness
     MAX_PIECE_NAME_BUFFER_SIZE = 63,          // the whole path counts not just the file name
     HIGHLIGHT_COLOR_AMOUNT = 30,              // the bigger the number the more the highlight effect is visible
     MAX_VALID_COLOR = 255,                    // don't change this it's not customizable this is how colors work
     VALID_MOVE_CIRCLE_SQUARE_COEFFICIENT = 3, // the bigger the number the smaller the circle

     // main.c
     START_SCREEN_WIDTH = 1280,
     START_SCREEN_HEIGHT = 720,
     MIN_SCREEN_WIDTH = 480,
     FPS = 60,

};
