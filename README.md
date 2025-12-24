# ♟️ Raylib Chess

A fully featured, C-based Chess implementation using the **raylib** game programming library.

This project goes beyond simple rendering, implementing a complete chess engine with move validation, special rules (Castling, En Passant, Promotion), game history (Undo/Redo), and FEN state serialization.

---

## ✨ Features

### Core Gameplay
- **Complete Move Validation:** Prevents illegal moves, including pinning pieces and moving the King into check.
- **Special Moves:**
  - 🏰 **Castling** (King-side and Queen-side).
  - ♟️ **En Passant** captures.
  - 👑 **Pawn Promotion** (UI allows selection of Queen, Rook, Bishop, or Knight).
- **Game States:** Detects **Check**, **Checkmate**, **Stalemate**, and **Insufficient Material**.
- **Draw Rules:** Supports 3-fold repetition detection.

### System & UI
- **Save & Load:** Full support for **FEN (Forsyth–Edwards Notation)** strings.
  - Saves board state, active color, castling rights, en passant targets, and move clocks.
- **History:** Unlimited **Undo/Redo** functionality using dynamic stacks.
- **Audio:** Sound effects for moves, captures, checks, and checkmate.
- **Visuals:**
  - Valid move highlighting (smart borders/dots).
  - Last move highlighting.
  - Dynamic board resizing.
  - **Debug Mode:** Integrated debug overlay for developer insights.

---

## 🛠️ Build & Installation

### Requirements
- **OS:** Linux (Tested on Ubuntu/Debian) (it should also work for mac and windows)
- **Compiler:** GCC (C17 standard)
- **Tools:** Make, CMake (3.14+)

### Option 1: Automatic Build (Recommended)
This method uses CMake to automatically download, compile, and link **Raylib** locally for this project. No system-wide installation is required.

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure (downloads Raylib)
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Build
make

# 4. Run
./chess
```

### Option 2: Manual Build (Advanced)
For manual installation of Raylib system-wide:

```bash
git clone https://github.com/raysan5/raylib.git
cd raylib
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=ON
make -j$(nproc)
sudo make install
sudo ldconfig
```

Notes:
- The cmake step enables shared libraries so your project can link with -lraylib.
- If you prefer a local (non-sudo) install, use CMAKE_INSTALL_PREFIX when running cmake and update LD_LIBRARY_PATH accordingly.

---

## 📂 Project layout

- `main.c`      — program entry, window setup and main loop
- `main.h`      — core types (Piece, Cell, PieceType, Team)
- `draw.c/.h`   — board layout, piece loading, drawing helpers and simple input selection handling
- `move.c/.h`   — MovePiece, validation logic, and special moves (Castling, En Passant)
- `load.c/.h`   — FEN reader (ReadFEN)
- `save.c/.h`   — FEN writer (SaveFEN)
- `stack.c/.h`  — Dynamic stack implementation for Undo/Redo history
- `utils.c/.h`  — High-level game management (Restart, LoadGameFromFEN)
- `hash.c/.h`   — MD5 hashing for board state (repetition detection)
- `settings.h`  — Compile-time constants and configuration
- `colors.c/.h`    — ColorPair palette and named colors
- `style_amber.h` — UI styling definitions
- `assets/`     — put piece images here (see naming below)
- `Makefile`    — build rules

---

## 📜 Public API / Important functions

(The declarations appear in the project's headers; use these from main.c)

- `void DrawBoard(int ColorTheme);`
  - Compute layout for the current render size and draw the board and pieces. Called each frame inside BeginDrawing()/EndDrawing().

- `void LoadPiece(int row, int col, PieceType type, Team team);`
  - Load/assign a piece texture into GameBoard[row][col]. Existing texture is released before assignment.

- `int ComputeSquareLength(void);`
  - Returns the computed pixel size of a single board square for the current render resolution.

- `void InitializeBoard(void);`
  - Reset all GameBoard cells to empty and set their row/col indices. Call at startup or before loading a new position.

- `void UnloadBoard(void);`
  - Unload all textures held by GameBoard and set every cell empty. Call on shutdown or before replacing assets.

- `void ReadFEN(const char *FENstring, int size);`
  - Parse a FEN piece-placement string and populate GameBoard using LoadPiece.

- `void MovePiece(int initialRow, int initialCol, int finalRow, int finalCol);`
  - Move a piece between cells. Performs bounds checking and validates source presence. Uses LoadPiece for destination and SetEmptyCell for the source.

- `void SetEmptyCell(Cell *cell);`
  - Clear a cell and unload any associated texture.

Notes:
- DrawBoard also includes simple interactive selection handling (two-click select + move). The UI helpers manage highlight borders (selected / last move).
- MovePiece now performs bounds checking and logs a warning on invalid indices.

---

## 🖼️ Assets / Filenames

Place PNG images in `assets/` (or the path your code expects). Recommended naming convention the code uses (case-sensitive):

- `pawnW.png`, `pawnB.png`
- `kingW.png`, `kingB.png`
- `queenW.png`, `queenB.png`
- `rookW.png`, `rookB.png`
- `bishopW.png`, `bishopB.png`
- `knightW.png`, `knightB.png`

Filenames are case- and space-sensitive on Linux. The loader expects exact names; trim accidental spaces.

---

## 📚 Usage notes & API

Call order (high level):
1. Set config flags and call `InitWindow(...)` in `main.c`.
2. Call `InitializeBoard()` once at startup (sets up cell indices).
3. After the window exists compute layout size using `ComputeSquareLength()` if needed.
4. Load piece textures with `LoadPiece(row, col, type, team)`, or populate the board from a FEN string using `ReadFEN(...)`.
5. Render each frame with `DrawBoard(theme)` (which also handles simple mouse selection and move attempts).

Saving API (in `save.h`):
- `char *SaveFEN(void);` — serialize the in-memory board to a heap-allocated FEN-like string (caller must free)

Resource ownership:
- Textures loaded for pieces are stored in `GameBoard[row][col].piece.texture`. When replacing textures, `LoadPiece` will `UnloadTexture()` the old texture; call `UnloadBoard()` on shutdown to free remaining textures.

Resizing:
- Recompute layout (square length and cell positions) after window resize. Use `IsWindowResized()` or compare `GetRenderWidth()` / `GetRenderHeight()` to detect changes.

Debugging tips:
- If a texture fails to show:
  - Verify the working directory and that `assets/<name>.png` exists.
  - Check `GameBoard[row][col].piece.texture.id` after loading — zero means failure.
  - Use `TraceLog(LOG_INFO, ...)` to print load attempts and positions.
  - Draw a visible fallback rectangle when `texture.id == 0` to confirm the drawing code runs.

---

## 💾 Saving board state (FEN-like serialization)

The project provides a helper function `SaveFEN()` that serializes the current in-memory board
(`GameBoard[8][8]`) into a FEN-like string. This string encodes the pieces and empty squares
for each rank, separated by '/'.

Key points:
- Function: `char *SaveFEN(void)`
- Returns: A heap-allocated C string containing the FEN-like board representation.
- Caller must `free()` the returned pointer when finished.
- On error (allocation failure or invalid piece type), the function returns `NULL`.

Current format produced:
- Ranks are written from top (row 0) to bottom (row 7).
- Pieces are represented by letters: `k,q,r,b,n,p`
  - lowercase = black, UPPERCASE = white
- Empty squares in a rank are compressed into digits 1–8.
- Example rank: `"rnbqkbnr"` or `"8"` or `"r1bqkb1r"`

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

---

## 📝 Coding style & documentation

- Headers are self-contained — include `raylib.h` in headers that need it.
- Prefer `snprintf()` over `strcpy/strcat` and trim filenames before loading.
- Use explicit-width integer types (`uint8_t`, `int16_t`) when size matters; `char` works for small flags.
- New/modified functions are documented with Doxygen-style comments in their .c files.

---

## 🤝 Contributing

- Add small, focused commits.
- Document any new exported function in the corresponding header with a short comment describing inputs, outputs and side effects.
