/**
 * load.h
 *
 * Responsibilities:
 * - Export the FEN parsing functionality.
 */

#ifndef LOAD_H
#define LOAD_H

#include <stdbool.h>
#include <stddef.h>

/* Parses a FEN string and optionally loads the game state */
bool ReadFEN(const char *FENstring, size_t size, bool testInputStringOnly);

#endif /* LOAD_H */