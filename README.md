# Chess (raylib) — Project README

Small chess demo using raylib. This repository contains rendering, piece loading, simple board layout, FEN parsing, and basic input handling.

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
- draw.c/.h   — board layout, piece loading, drawing helpers and simple input selection handling
- main.h      — core types (Piece, Cell, PieceType, Team)
- colors.h    — ColorPair palette and named colors
- load.c/.h   — FEN reader (ReadFEN)
- move.c/.h   — MovePiece and SetEmptyCell helpers
- assets/     — put piece images here (see naming below)
- Makefile    — build rules

## Public API / Important functions

(The declarations appear in the project's headers; use these from main.c)

- void DrawBoard(int ColorTheme);
  - Compute layout for the current render size and draw the board and pieces. Called each frame inside BeginDrawing()/EndDrawing().

- void LoadPiece(int row, int col, PieceType type, Team team);
  - Load/assign a piece texture into GameBoard[row][col]. Existing texture is released before assignment.

- int ComputeSquareLength(void);
  - Returns the computed pixel size of a single board square for the current render resolution.

- void InitializeBoard(void);
  - Reset all GameBoard cells to empty and set their row/col indices. Call at startup or before loading a new position.

- void UnloadBoard(void);
  - Unload all textures held by GameBoard and set every cell empty. Call on shutdown or before replacing assets.

- void ReadFEN(const char *FENstring, int size);
  - Parse a FEN piece-placement string and populate GameBoard using LoadPiece.

- void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);
  - Move a piece between cells. Performs bounds checking and validates source presence. Uses LoadPiece for destination and SetEmptyCell for the source.

- void SetEmptyCell(Cell *cell);
  - Clear a cell and unload any associated texture.

Notes:
- DrawBoard also includes simple interactive selection handling (two-click select + move). The UI helpers manage highlight borders (selected / last move).
- MovePiece now performs bounds checking and logs a warning on invalid indices.

## Assets / Filenames

Place PNG images in `assets/` (or the path your code expects). Recommended naming convention the code uses (case-sensitive):

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
2. Call `InitializeBoard()` once at startup (sets up cell indices).
3. After the window exists compute layout size using `ComputeSquareLength()` if needed.
4. Load piece textures with `LoadPiece(row, col, type, team)`, or populate the board from a FEN string using `ReadFEN(...)`.
5. Render each frame with `DrawBoard(theme)` (which also handles simple mouse selection and move attempts).

Resource ownership:
- Textures loaded for pieces are stored in `GameBoard[row][col].piece.texture`. When replacing textures, `LoadPiece` will `UnloadTexture()` the old texture; call `UnloadBoard()` on shutdown to free remaining textures.

Resizing:
- Recompute layout (square length and cell positions) after window resize. Use `IsWindowResized()` or compare `GetRenderWidth()` / `GetRenderHeight()` to detect changes.

Drawing:
- For best visual quality when scaling textures, use `DrawTexturePro` and set linear filtering with `SetTextureFilter(tex, FILTER_BILINEAR)` or enable MSAA via `SetConfigFlags(FLAG_MSAA_4X_HINT)` before `InitWindow()`.

Debugging tips:
- If a texture fails to show:
  - Verify the working directory and that `assets/<name>.png` exists.
  - Check `GameBoard[row][col].piece.texture.id` after loading — zero means failure.
  - Use `TraceLog(LOG_INFO, ...)` to print load attempts and positions.
  - Draw a visible fallback rectangle when `texture.id == 0` to confirm the drawing code runs.

## Coding style & documentation

- Headers are self-contained — include `raylib.h` in headers that need it.
- Prefer `snprintf()` over `strcpy/strcat` and trim filenames before loading.
- Use explicit-width integer types (`uint8_t`, `int16_t`) when size matters; `char` works for small flags.
- New/modified functions are documented with Doxygen-style comments in their .c files.

## Contributing

- Add small, focused commits.
- Document any new exported function in the corresponding header with a short comment describing inputs, outputs and side effects.
