#ifndef UTILS_H
#define UTILS_H

void RestartGame(void);

// NEW: Helper to reset state and load a specific FEN
void LoadGameFromFEN(const char *fen);

#endif