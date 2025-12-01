# Chess (raylib) — Project README

Small chess demo using raylib. This repository contains rendering, piece loading and simple board layout code.

## Requirements

- Linux
- gcc (C17)
- raylib (dev headers + library)
- make

Install raylib on Ubuntu/Debian (example):

Building and installing raylib (GNU/Linux)
1. Install the required development tools and libraries using your distribution's package manager (install cmake, git, build tools and the usual X/OpenGL development packages).
2. Build and install raylib from source:

```bash
git clone https://github.com/raysan5/raylib.git
cd raylib
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=ON
make -j$(nproc)
sudo make install
sudo ldconfig      # refresh linker cache (if necessary)
```

Notes:
- The cmake step enables shared libraries so your project can link with -lraylib.
- If you prefer a local (non-sudo) install, use CMAKE_INSTALL_PREFIX when running cmake and update LD_LIBRARY_PATH accordingly.

## Build

From project root (/home/omar/Project/chess):

- Build (release):
  ```
  make
  ```

- Build (debug):
  ```
  make debug
  ```

- Run the release build:
  ```
  make run
  ```

- Run the debug build:
  ```
  make run-debug
  ```

Binaries and object files are placed under `build/`.

## Project layout

- main.c      — program entry, window setup and main loop
- draw.c/.h   — board layout, piece loading and drawing helpers
- main.h      — core types (Piece, Cell, PieceType, Team)
- colors.h    — ColorPair palette and named colors
- load.c/.h    — Add the ability to read FEN chess format
- assets/     — put piece images here (see naming below)
- Makefile    — build rules

## Assets / Filenames

Place PNG images in `assets/` (or the path your code expects). Recommended naming convention the code uses:

- pawnW.png, pawnB.png
- kingW.png, kingB.png
- queenW.png, queenB.png
- rookW.png, rookB.png
- bishopW.png, bishopB.png
- knightW.png, knightB.png

Filenames are case- and space-sensitive on Linux. The loader expects exact names; trim accidental spaces.

## Usage notes & API

Call order (high level):
1. Set config flags and call `InitWindow(...)` in `main.c`.
2. After the window exists compute layout size using `ComputeSquareLength()` and/or your layout helper.
3. Load piece textures with `LoadPiece(row, col, type, team, squareLength)`.
4. Render each frame with `DrawBoard(theme)` (which uses the board/piece state).

Important exported functions (in `draw.h`):
- `void DrawBoard(int ColorTheme);`
- `void LoadPiece(int row, int col, PieceType type, Team team, int squareLength);`
- `int ComputeSquareLength(void);`

Resource ownership:
- Textures loaded for pieces are stored in `GameBoard[row][col].piece.texture`. When replacing textures, call `UnloadTexture()` on the old texture to avoid GPU leaks.

Resizing:
- Recompute layout (square length and cell positions) after window resize. Use `IsWindowResized()` or compare `GetRenderWidth()` / `GetRenderHeight()` to detect changes.

Drawing:
- For best visual quality when scaling textures, use `DrawTexturePro` and set linear filtering with `SetTextureFilter(tex, FILTER_BILINEAR)` or enable MSAA via `SetConfigFlags(FLAG_MSAA_4X_HINT)` before `InitWindow()`.

Debugging tips
- If a texture fails to show:
  - Verify the working directory and that `assets/<name>.png` exists.
  - Check `GameBoard[row][col].piece.texture.id` after loading — zero means failure.
  - Use `TraceLog(LOG_INFO, ...)` to print load attempts and positions.
  - Draw a visible fallback rectangle when texture.id == 0 to confirm the drawing code runs.

## Saving board state (FEN-like serialization)

The project provides a helper function SaveFEN() that serializes the current in-memory board
(GameBoard[8][8]) into a FEN-like string. This string encodes the pieces and empty squares
for each rank, separated by '/'.

Key points:
- Function: char *SaveFEN(void)
- Returns: A heap-allocated C string containing the FEN-like board representation.
- Caller must free() the returned pointer when finished.
- On error (allocation failure or invalid piece type), the function returns NULL.

Current format produced:
- Ranks are written from top (row 0) to bottom (row 7).
- Pieces are represented by letters: k,q,r,b,n,p
  - lowercase = black, UPPERCASE = white
- Empty squares in a rank are compressed into digits 1–8.
- Example rank: "rnbqkbnr" or "8" or "r1bqkb1r"
- The produced string does not yet include side-to-move, castling rights,
  en-passant target, halfmove clock or fullmove number.

Example usage:

```c
// Example: save the current board to a string and print it
char *fen = SaveFEN();
if (fen != NULL) {
    printf("Board FEN: %s\n", fen);
    free(fen);
} else {
    fprintf(stderr, "Failed to produce FEN string\n");
}
```

TODO:
- Append side-to-move ('w' or 'b') to the FEN string.
- Optionally add castling, en-passant and move counters to match full FEN.

## Coding style & documentation

- Headers are self-contained — include types (`raylib.h`) in headers that need them.
- Prefer `snprintf()` over `strcpy/strcat` and trim filenames before loading.
- Use explicit-width integer types (`uint8_t`, `int16_t`) when size matters; `char` works for small flags.

## Contributing

- Add small, focused commits.
- Document any new exported function in the corresponding header with a short comment describing inputs, outputs and side effects.
