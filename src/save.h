/**
 * save.h
 *
 * Responsibilities:
 * - Export the FEN generation functionality.
 */

#ifndef SAVE_H
#define SAVE_H

/*
 * SaveFEN
 *
 * Serializes the current game state into a FEN string.
 *
 * Returns: A pointer to a string containing the FEN representation of the
 *          current game state. The caller is responsible for freeing the
 *          allocated memory.
 */

unsigned char *SaveFEN(void);

#endif