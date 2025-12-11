# Makefile for ScisSOS (objects go to obj/)

# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Wextra -g -O2
LDFLAGS := -L. -lscismem -lm

# Object directory
OBJ := obj

# Sources and objects
SOURCES := main.c os.c process.c scheduling_algo.c memory.c
OBJECTS := $(addprefix $(OBJ)/, $(SOURCES:.c=.o))
HEADERS := ScisSos.h scheduling_algo.h

# Executables
TARGET      := run_os
TEST_TARGET := test_perf
MEM_TEST    := test_memory_gen

# Default target
.PHONY: all
all: $(TARGET)

# Create object dir if missing
$(OBJ):
	@mkdir -p $(OBJ)

# Link main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Link test executable (performance)
$(TEST_TARGET): $(OBJ)/test_perf.o $(OBJ)/os.o $(OBJ)/process.o $(OBJ)/scheduling_algo.o $(OBJ)/memory.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TEST_TARGET)"

# Link memory test
$(MEM_TEST): $(OBJ)/test_memory_gen.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(MEM_TEST)"

# Compile .c -> obj/.o (depends on headers)
$(OBJ)/%.o: %.c $(HEADERS) | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET) $(TEST_TARGET) $(MEM_TEST)
	@echo "Clean complete"

# ---------------------------
# Convenience run targets
# ---------------------------

# Default scheduler/run variables for the flexible 'run' target
SCHED ?= rr
EXP   ?=
FRAMES ?= 64

# Generic run target:
# - If EXP is empty, only SCHED is passed.
# - If EXP is set, pass SCHED EXP FRAMES.
.PHONY: run
run: $(TARGET)
	./$(TARGET) $(SCHED) $(if $(EXP),$(EXP) $(FRAMES))

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

# Experiment 1: Compare with/without memory manager
.PHONY: experiment1_nomem experiment1_mem
experiment1_nomem: $(TARGET)
	@echo "=== Experiment 1a: Without Memory Manager ==="
	./$(TARGET) rr > results_exp1_nomem.txt

experiment1_mem: $(TARGET)
	@echo "=== Experiment 1b: With Memory Manager ==="
	./$(TARGET) rr > results_exp1_mem.txt

# Experiment 2: Vary frame count (single-run fallback)
.PHONY: experiment2
experiment2: $(TARGET)
	@echo "=== Experiment 2: Varying Frame Count ==="
	./$(TARGET) rr > results_exp2_f_all.txt
	@echo "Results saved to results_exp2_f_all.txt"

# Flexible experiment run using variables:
# Example usage:
#   make run SCHED=rr EXP=2 FRAMES=64
# This will call: ./run_os rr 2 64
.PHONY: run_with_params
run_with_params: $(TARGET)
	@if [ -z "$(EXP)" ]; then \
	  echo "EXP not set; use 'make run SCHED=rr EXP=1 FRAMES=64' to pass experiment/frame args"; \
	else \
	  ./$(TARGET) $(SCHED) $(EXP) $(FRAMES); \
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
	@echo "  all              - Build main executable (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  test_memgen      - Test ScisSosMem library integration"
	@echo "  run_fcfs         - Run with FCFS scheduler (single-arg)"
	@echo "  run_sjf          - Run with SJF scheduler (single-arg)"
	@echo "  run_priority     - Run with Priority scheduler (single-arg)"
	@echo "  run_rr           - Run with Round Robin scheduler (single-arg)"
	@echo "  run              - Flexible run target: ./run_os SCHED [EXP FRAMES]"
	@echo "                     Example: make run SCHED=rr"
	@echo "                     Example: make run SCHED=rr EXP=1 FRAMES=64"
	@echo "  run_with_params  - Alternative: checks EXP and runs with params"
	@echo "  experiment1_mem  - Run experiment 1 (memory manager ON)"
	@echo "  experiment2      - Run experiment 2 (varying frames) - simplified"
	@echo ""
	@echo "Manual usage examples:"
	@echo "  make run SCHED=rr"
	@echo "  make run SCHED=rr EXP=1 FRAMES=64"
	@echo "  make test_memgen"

.PHONY: all clean run run_fcfs run_sjf run_priority run_rr experiment1_nomem experiment1_mem experiment2 run_test_perf test_memgen help
