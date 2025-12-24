/**
 * settings.h
 *
 * Responsibilities:
 * - Define compile-time constants and configuration settings for the game.
 * - Group settings by module (draw, move, main) for better organization.
 * - Provide constants for board dimensions, UI layout, and game rules.
 *
 * Notes:
 * - These values are fixed at build time and cannot be changed at runtime.
 * - Includes settings for visual customization (fonts, highlights), game logic (castling ranks),
 *   and application defaults (screen size, FPS).
 */

#ifndef SETTINGS_H
#define SETTINGS_H

/*
 * Global Game Settings
 *
 * These enumerations define various constants used throughout the application.
 * They are grouped loosely by the source file or subsystem they primarily affect.
 */
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
     DEBUG_MENU_FONT_SIZE = 20,
     SPACE_BETWEEN_DEBUG_LINES = 2,
     SPACE_BETWEEN_DEBUG_SECTIONS = 5,
     DEBUG_INFO_WINDOW_WIDTH = 300,
     DEBUG_INFO_WINDOW_HEIGHT = 470,

     /* Status bar settings */
     STATUS_MENU_FONT_SIZE = 30,
     STATUS_MENU_PADDING = 20,

     UI_FONT = 20,

     // --- MOVE.C SETTINGS (Game Logic) ---

     /* Castling Ranks (0-7) */
     WHITE_BACK_RANK = 7,
     BLACK_BACK_RANK = 0,

     /* Initial Piece Columns */
     KING_START_COL = 4,
     ROOK_QS_COL = 0, // Queen Side Rook Start (a-file)
     ROOK_KS_COL = 7, // Queen Side Rook Start (a-file)

     /* Castling Destination Columns (King) */
     CASTLE_KS_KING_COL = 6, // g-file
     CASTLE_QS_KING_COL = 2, // c-file

     /* Castling Destination Columns (Rook) */
     CASTLE_KS_ROOK_COL = 5, // f-file
     CASTLE_QS_ROOK_COL = 3, // d-file

     // --- MAIN.C SETTINGS (Application & UI) ---

     /* Window defaults */
     START_SCREEN_WIDTH = 1280,
     START_SCREEN_HEIGHT = 720,
     MIN_SCREEN_WIDTH = 480,
     FPS = 60,

     /* Memory allocation defaults */
     INITIAL_DYNAMIC_HASH_ARRAY_SIZE = 32,
     INITAL_UNDO_REDO_STACK_SIZE = 32,
     MAX_FILE_NAME_LENGTH = 64,

     /* Popup UI Dimensions */
     POPUP_INPUT_WIDTH = 240,
     POPUP_INPUT_HEIGHT = 140,
     POPUP_OVERWRITE_WIDTH = 250,
     POPUP_OVERWRITE_HEIGHT = 100,
     POPUP_WRONG_FEN_WIDTH = 250,
     POPUP_WRONG_FEN_HEIGHT = 100,
     POPUP_LOAD_WIDTH = 300,
     POPUP_LOAD_HEIGHT = 320,
     MAX_LOAD_FILES_TOTAL_LENGTH = 4096,
     POPUP_FEN_WIDTH = 300,
     POPUP_FEN_HEIGHT = 125,
     POPUP_GAMEOVER_WIDTH = 300,
     POPUP_GAMEOVER_HEIGHT = 160,
     GAMEOVER_FONT_SIZE = 20,
};

/* Board Dimensions */
#define BOARD_SIZE 8

#define FADE_CONSTANT 0.75f

/* Standard Chess Starting Position (FEN) */
#define STARTING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#endif /* SETTINGS_H */