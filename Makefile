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
SRC := main.c draw.c load.c move.c
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

# LDFLAGS: Linker Flags
# The compiler finds libraylib.so in the standard system path automatically
LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

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
