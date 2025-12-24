/**
 * utils.h
 *
 * Responsibilities:
 * - Export high-level game management functions.
 */

#ifndef UTILS_H
#define UTILS_H

/* Resets the game to the standard starting position */
void RestartGame(void);

/* Helper to reset state and load a specific FEN */
void LoadGameFromFEN(const char *fen);

#endif