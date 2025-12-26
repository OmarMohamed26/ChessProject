/*
 * colors.h
 *
 * Responsibilities:
 * - Centralize color definitions and named color constants used across the project.
 * - Define the ColorPair structure for board themes.
 * - Define the ColorTheme enum for selecting themes.
 *
 * Conventions:
 * - Macros define Color literals; treat them as read-only values.
 * - Filenames / other modules reference these macros or build ColorPair arrays.
 */
#ifndef COLORS_H
#define COLORS_H

#include "raylib.h"

// --- Board Theme Colors ---
// Each theme has a 'White' (light square) and 'Black' (dark square) definition.

#define BROWN_WHITE CLITERAL(Color){242, 212, 174, 255}
#define BROWN_BLACK CLITERAL(Color){192, 132, 98, 255}

#define GREEN_WHITE CLITERAL(Color){235, 236, 208, 255}
#define GREEN_BLACK CLITERAL(Color){115, 149, 82, 255}

#define ORANGE_WHITE CLITERAL(Color){255, 226, 172, 255}
#define ORANGE_BLACK CLITERAL(Color){221, 132, 24, 255}

#define PURPLE_WHITE CLITERAL(Color){239, 241, 240, 255}
#define PURPLE_BLACK CLITERAL(Color){132, 118, 185, 255}

#define RED_WHITE CLITERAL(Color){250, 217, 193, 255}
#define RED_BLACK CLITERAL(Color){200, 83, 71, 255}

#define SKY_WHITE CLITERAL(Color){239, 241, 240, 255}
#define SKY_BLACK CLITERAL(Color){190, 215, 227, 255}

// --- UI & Highlight Colors ---

// #define BACKGROUND CLITERAL(Color){48, 46, 43, 255} // This was used before we added the amber style it's no longer needed
#define FONT_COLOR RAYWHITE

// Highlight for the currently selected piece's square
#define SELECTED_BORDER_COLOR CLITERAL(Color){255, 203, 0, 150}

// Highlight for the source and destination of the last move
#define LAST_MOVE_BORDER_COLOR CLITERAL(Color){0, 228, 48, 150}

// Indicator for valid move destinations (dots/circles)
#define VALID_MOVE_COLOR CLITERAL(Color){100, 100, 100, 100}

#define DEBUG_TEXT_COLOR WHITE
#define STATUS_TEXT_COLOR WHITE

/**
 * ColorPair
 *
 * Represents the two colors used for the checkerboard pattern of a theme.
 */
typedef struct ColorPair
{
    Color white; // Color for light squares
    Color black; // Color for dark squares
} ColorPair;

/**
 * ColorTheme
 *
 * Enumeration of available board color themes.
 * Used as an index into the global PALETTE array.
 */
typedef enum
{
    THEME_BROWN,
    THEME_GREEN,
    THEME_ORANGE,
    THEME_PURPLE,
    THEME_RED,
    THEME_SKY,
    THEME_COUNT
} ColorTheme;

extern const ColorPair PALETTE[THEME_COUNT];

#endif