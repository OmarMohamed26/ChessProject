/**
 * colors.c
 *
 * Responsibilities:
 * - Define the global color palette used for board themes.
 * - Maps the ColorTheme enum values to specific ColorPair structures.
 */

#include "colors.h"

/**
 * PALETTE
 *
 * Global array of ColorPairs indexed by the ColorTheme enum.
 * Each entry defines the 'white' (light) and 'black' (dark) square colors
 * for a specific visual theme.
 */
const ColorPair PALETTE[THEME_COUNT] = {
    {BROWN_WHITE, BROWN_BLACK},   /* THEME_BROWN  */
    {GREEN_WHITE, GREEN_BLACK},   /* THEME_GREEN  */
    {ORANGE_WHITE, ORANGE_BLACK}, /* THEME_ORANGE */
    {PURPLE_WHITE, PURPLE_BLACK}, /* THEME_PURPLE */
    {RED_WHITE, RED_BLACK},       /* THEME_RED    */
    {SKY_WHITE, SKY_BLACK}        /* THEME_SKY    */
};
