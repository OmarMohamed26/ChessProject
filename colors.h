/*
 * colors.h
 *
 * Small header that centralizes color definitions and named color constants
 * used across the project. Keep this header minimal and include <raylib.h>
 * so consumers have the Color type available.
 *
 * Conventions:
 * - Macros define Color literals; treat them as read-only values.
 * - Filenames / other modules reference these macros or build ColorPair arrays.
 */
#ifndef COLORS_H
#define COLORS_H

#include "raylib.h"

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

#define BACKGROUND CLITERAL(Color){48, 46, 43, 255}

#define FONT_COLOR RAYWHITE

#define SELECTED_BORDER_COLOR CLITERAL(Color){255, 203, 0, 150}

#define LAST_MOVE_BORDER_COLOR CLITERAL(Color){0, 228, 48, 150}

#define VALID_MOVE_COLOR CLITERAL(Color){100, 100, 100, 100}

#define DEBUG_TEXT_COLOR WHITE

typedef struct ColorPair
{
    Color white;
    Color black;
} ColorPair;

/* Palette enum + array of pairs */
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