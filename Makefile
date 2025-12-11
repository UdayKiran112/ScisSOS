# Makefile for ScisSOS (objects go to obj/)

# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Wextra -g -O2
LDFLAGS := -L. -lscismem -lm

# Object directory
OBJ := obj

# Sources and objects
SOURCES := main.c os.c process.c scheduling_algo.c memory.c
# Also allow test/experiment sources to be compiled into obj/
TEST_SOURCES := test_perf.c test_memory_gen.c experiment.c

# Build list of object files in obj/
OBJECTS := $(addprefix $(OBJ)/, $(SOURCES:.c=.o))
TEST_OBJECTS := $(addprefix $(OBJ)/, $(TEST_SOURCES:.c=.o))

# Headers
HEADERS := ScisSos.h scheduling_algo.h

# Executables
TARGET      := run_os
EXP_TARGET  := experiment
TEST_TARGET := test_perf
MEM_TEST    := test_memory_gen

# Default target
.PHONY: all
all: $(TARGET) $(EXP_TARGET)

# Create object dir if missing (order-only prerequisite)
$(OBJ):
	@mkdir -p $(OBJ)

# Compile rule: .c -> obj/.o
# Order-only dependency on $(OBJ) so dir exists
$(OBJ)/%.o: %.c $(HEADERS) | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# Link main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Link experiment executable
# experiment.c is a separate driver that needs the OS objects
$(EXP_TARGET): $(OBJ)/experiment.o $(OBJ)/os.o $(OBJ)/process.o $(OBJ)/scheduling_algo.o $(OBJ)/memory.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(EXP_TARGET)"

# Link test executable (performance)
$(TEST_TARGET): $(OBJ)/test_perf.o $(OBJ)/os.o $(OBJ)/process.o $(OBJ)/scheduling_algo.o $(OBJ)/memory.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TEST_TARGET)"

# Link memory test
$(MEM_TEST): $(OBJ)/test_memory_gen.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(MEM_TEST)"

# Clean
.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET) $(EXP_TARGET) $(TEST_TARGET) $(MEM_TEST)
	@echo "Clean complete"

# ---------------------------
# Convenience run targets
# ---------------------------

# Default scheduler/run variables for the flexible 'run' target
SCHED ?= rr
EXP   ?=
FRAMES ?= 64

# Generic run target:
# - If EXP is empty, run the main simulator: ./run_os <scheduler>
# - If EXP is set, run the experiment driver: ./experiment <scheduler> <EXP> <FRAMES>
.PHONY: run
run: $(TARGET) $(EXP_TARGET)
ifeq ($(strip $(EXP)),)
	@echo "Running main simulator: ./$(TARGET) $(SCHED)"
	./$(TARGET) $(SCHED)
else
	@echo "Running experiment driver: ./$(EXP_TARGET) $(SCHED) $(EXP) $(FRAMES)"
	./$(EXP_TARGET) $(SCHED) $(EXP) $(FRAMES)
endif

# Common one-liner scheduler helpers (single-arg calls)
.PHONY: run_fcfs run_sjf run_priority run_rr
run_fcfs: $(TARGET)
	./$(TARGET) fcfs

run_sjf: $(TARGET)
	./$(TARGET) sjf

run_priority: $(TARGET)
	./$(TARGET) priority

run_rr: $(TARGET)
	./$(TARGET) rr

# ---------------------------
# Experiment targets
# ---------------------------

# Experiment 1: Compare with/without memory manager (K frames)
.PHONY: experiment1_nomem experiment1_mem experiment1
experiment1: $(EXP_TARGET)
	@echo "=== Experiment 1 (interactive) - example usage below ==="
	@echo "make run SCHED=rr EXP=1 FRAMES=64"
	@echo "or: ./experiment rr 1 64"

# Shortcuts to run experiment driver with predefined args and save output
experiment1_nomem: $(EXP_TARGET)
	@echo "=== Experiment 1a: MEMORY OFF then ON (saved to results_exp1_k64.txt) ==="
	./$(EXP_TARGET) rr 1 64 > results_exp1_k64.txt

experiment1_mem: $(EXP_TARGET)
	@echo "=== Experiment 1b: MEMORY runs (varying) (saved to results_exp1_mem.txt) ==="
	./$(EXP_TARGET) rr 2 64 > results_exp1_mem.txt

# Experiment 2: Vary frame count (single-run fallback)
.PHONY: experiment2
experiment2: $(EXP_TARGET)
	@echo "=== Experiment 2: Varying Frame Count (driver will loop K=32..128) ==="
	./$(EXP_TARGET) rr 2 64 > results_exp2_f_all.txt
	@echo "Results saved to results_exp2_f_all.txt"

# Flexible experiment run using variables:
# Example usage:
#   make run SCHED=rr EXP=2 FRAMES=64
# This will call: ./experiment rr 2 64
.PHONY: run_with_params
run_with_params: $(EXP_TARGET)
	@if [ -z "$(EXP)" ]; then \
	  echo "EXP not set; use 'make run SCHED=rr EXP=1 FRAMES=64' to pass experiment/frame args"; \
	else \
	  ./$(EXP_TARGET) $(SCHED) $(EXP) $(FRAMES); \
	fi

# ---------------------------
# Tests & utilities
# ---------------------------

.PHONY: run_test_perf test_memgen
run_test_perf: $(TEST_TARGET)
	./$(TEST_TARGET)

test_memgen: $(MEM_TEST)
	./$(MEM_TEST)

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build main and experiment executables (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  test_memgen      - Test ScisSosMem library integration"
	@echo "  run_fcfs         - Run with FCFS scheduler (single-arg)"
	@echo "  run_sjf          - Run with SJF scheduler (single-arg)"
	@echo "  run_priority     - Run with Priority scheduler (single-arg)"
	@echo "  run_rr           - Run with Round Robin scheduler (single-arg)"
	@echo "  run              - Flexible run target:"
	@echo "                      If EXP is empty: './run_os <SCHED>'"
	@echo "                      If EXP is set:   './experiment <SCHED> <EXP> <FRAMES>'"
	@echo "                     Example: make run SCHED=rr"
	@echo "                     Example: make run SCHED=rr EXP=1 FRAMES=64"
	@echo "  run_with_params  - Alternative: checks EXP and runs with params"
	@echo "  experiment1      - Shortcut info for experiment 1"
	@echo "  experiment2      - Runs experiment2 and stores results"
	@echo ""
	@echo "Manual usage examples:"
	@echo "  make run SCHED=rr"
	@echo "  make run SCHED=rr EXP=1 FRAMES=64"
	@echo "  make run_with_params SCHED=rr EXP=2 FRAMES=64"

# Explicit phony list
.PHONY: all clean run run_fcfs run_sjf run_priority run_rr experiment1_nomem experiment1_mem experiment2 run_test_perf test_memgen help run_with_params experiment1
