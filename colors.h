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

#include <raylib.h>

#define BROWNWHITE CLITERAL(Color){242, 212, 174, 255}
#define BROWNBLACK CLITERAL(Color){192, 132, 98, 255}

#define GREENWHITE CLITERAL(Color){235, 236, 208, 255}
#define GREENBLACK CLITERAL(Color){115, 149, 82, 255}

#define ORANGEWHITE CLITERAL(Color){255, 226, 172, 255}
#define ORANGEBLACK CLITERAL(Color){221, 132, 24, 255}

#define PURPLEWHITE CLITERAL(Color){239, 241, 240, 255}
#define PURPLEBLACK CLITERAL(Color){132, 118, 185, 255}

#define REDWHITE CLITERAL(Color){250, 217, 193, 255}
#define REDBLACK CLITERAL(Color){200, 83, 71, 255}

#define SKYWHITE CLITERAL(Color){239, 241, 240, 255}
#define SKYBLACK CLITERAL(Color){190, 215, 227, 255}

#define BACKGROUND CLITERAL(Color){48, 46, 43, 255}

#define FONT_COLOR RAYWHITE

#define SELECTED_BORDER_COLOR GOLD
#define LAST_MOVE_BORDER_COLOR GREEN

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

static const ColorPair PALETTE[THEME_COUNT] = {
    {BROWNWHITE, BROWNBLACK},   /* THEME_BROWN  */
    {GREENWHITE, GREENBLACK},   /* THEME_GREEN  */
    {ORANGEWHITE, ORANGEBLACK}, /* THEME_ORANGE */
    {PURPLEWHITE, PURPLEBLACK}, /* THEME_PURPLE */
    {REDWHITE, REDBLACK},       /* THEME_RED    */
    {SKYWHITE, SKYBLACK}        /* THEME_SKY    */
};

#endif