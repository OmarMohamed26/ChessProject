# --- Build Configuration Variables ---
TARGET ?= chess
CC := gcc
BUILD_DIR := build
# Default build mode is Release (Optimized)
BUILD_MODE ?= Release

# If the user requested the 'debug' goal, set Debug mode early (affects parsing)
ifneq (,$(filter debug,$(MAKECMDGOALS)))
BUILD_MODE := Debug
TARGET := debugChess
endif

# Conditional CFLAGS based on the mode
ifeq ($(BUILD_MODE), Debug)
CFLAGS += -g -O0 -DDEBUG
$(info Building in DEBUG mode (-g -O0, C17)...)
else
CFLAGS += -O3 -s
$(info Building in RELEASE mode (-O2, C17)...)
endif

# Source and Generated Files
# Add all your .c files here
SRC := main.c draw.c load.c save.c move.c colors.c
OBJ := $(addprefix $(BUILD_DIR)/$(BUILD_MODE)/, $(SRC:.c=.o))
DEP := $(addprefix $(BUILD_DIR)/$(BUILD_MODE)/, $(SRC:.c=.d))
EXECUTABLE = $(BUILD_DIR)/$(TARGET)

# --- Compiler Flags ---

# CFLAGS: Common Flags (System-wide Setup)
# -Wall, -Wextra: Robust warnings
# -std=c17: Modern C standard
# -MMD: Auto-generate dependency files
# Note: Removed -I. because system headers (raylib.h) are found automatically
CFLAGS += -Wall -Wextra -std=c17 -MMD

# --- Library / Include directory support ---
# Add custom library dirs and include dirs when needed:
#   make LIB_DIRS="/opt/local/lib /usr/local/lib" INCLUDE_DIRS="/opt/local/include" LIBS="raylib GL m pthread dl rt X11"
LIB_DIRS ?= libs
INCLUDE_DIRS ?= includes
# Space-separated library names (without -l prefix). Defaults keep previous behaviour.
LIBS ?= raylib GL m pthread dl rt X11

# Expand include dirs into -I flags and library dirs/libs into -L / -l flags
ifneq ($(strip $(INCLUDE_DIRS)),)
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
endif

# Build LDFLAGS and add rpath entries when LIB_DIRS is set so the runtime can find .so there
LDFLAGS := $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS))
ifneq ($(strip $(LIB_DIRS)),)
# Add rpath entries for each LIB_DIR so the runtime loader can find shared libs there.
# Use foreach to produce one "-Wl,-rpath=DIR" token per directory (avoids addprefix comma parsing issues).
LDFLAGS += $(foreach d,$(LIB_DIRS),-Wl,-rpath=$(d))
endif
# --- Targets ---

.PHONY: all debug run clean

# Default Target: 'make' builds the optimized RELEASE version
all: $(BUILD_DIR)/$(BUILD_MODE) $(EXECUTABLE)

# Debug Target: 'make debug' builds the DEBUG version (target-specific vars)
debug: BUILD_MODE=Debug TARGET=debugChess
debug: all

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	@mkdir -p $@

# Create build subdir per mode
$(BUILD_DIR)/$(BUILD_MODE):
	@mkdir -p $@

# Final Linking
$(EXECUTABLE): $(OBJ)
	@echo "Linking $(EXECUTABLE)..."
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compiling .c to .o
$(BUILD_DIR)/$(BUILD_MODE)/%.o: %.c | $(BUILD_DIR)/$(BUILD_MODE)
	$(CC) $(CFLAGS) -c $< -o $@

# Include auto-generated dependency files
-include $(DEP)

# Run Target
run: $(EXECUTABLE)
	@echo "Running $(EXECUTABLE)..."
	./$(EXECUTABLE)

# Build the debug version and run it
.PHONY: run-debug
run-debug: debug
	@echo "Running debug build $(BUILD_DIR)/debugChess..."
	./$(BUILD_DIR)/debugChess

# Clean Target
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
