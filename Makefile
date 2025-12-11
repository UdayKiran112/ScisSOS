# Makefile for ScisSOS

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -L. -lscismem -lm

# Object directory
OBJ = obj

# Source files
SOURCES = main.c os.c process.c scheduling_algo.c memory.c
OBJECTS = $(addprefix $(OBJ)/, $(SOURCES:.c=.o))
HEADERS = ScisSos.h scheduling_algo.h

# Executables
TARGET = run_os
TEST_TARGET = test_perf
MEM_TEST = test_memory_gen

# Default target
all: $(TARGET)

# Ensure object directory exists
$(OBJ):
	mkdir -p $(OBJ)

# Build main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Build test executable
$(TEST_TARGET): $(OBJ)/test_perf.o $(OBJ)/os.o $(OBJ)/process.o $(OBJ)/scheduling_algo.o $(OBJ)/memory.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TEST_TARGET)"

# Build memory test
$(MEM_TEST): $(OBJ)/test_memory_gen.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(MEM_TEST)"

# Compile source files → OBJ directory
$(OBJ)/%.o: %.c $(HEADERS) | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ) $(TARGET) $(TEST_TARGET) $(MEM_TEST)
	@echo "Clean complete"

# Run targets with different schedulers
run_fcfs: $(TARGET)
	./$(TARGET) fcfs 1 64

run_sjf: $(TARGET)
	./$(TARGET) sjf 1 64

run_priority: $(TARGET)
	./$(TARGET) priority 1 64

run_rr: $(TARGET)
	./$(TARGET) rr 1 64

# Experiment 1: Compare with/without memory manager
experiment1_nomem: $(TARGET)
	@echo "=== Experiment 1a: Without Memory Manager ==="
	./$(TARGET) rr 1 64 > results_exp1_nomem.txt

experiment1_mem: $(TARGET)
	@echo "=== Experiment 1b: With Memory Manager ==="
	./$(TARGET) rr 1 64 > results_exp1_mem.txt

# Experiment 2: Vary frame count
experiment2: $(TARGET)
	@echo "=== Experiment 2: Varying Frame Count ==="
	./$(TARGET) rr 2 32 > results_exp2_f32.txt
	./$(TARGET) rr 2 48 > results_exp2_f48.txt
	./$(TARGET) rr 2 64 > results_exp2_f64.txt
	./$(TARGET) rr 2 80 > results_exp2_f80.txt
	./$(TARGET) rr 2 96 > results_exp2_f96.txt
	./$(TARGET) rr 2 112 > results_exp2_f112.txt
	./$(TARGET) rr 2 128 > results_exp2_f128.txt
	@echo "Results saved to results_exp2_*.txt"

# Run performance tests
run_test_perf: $(TEST_TARGET)
	./$(TEST_TARGET)

# Test memory generator library
test_memgen: $(MEM_TEST)
	./$(MEM_TEST)

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build main executable (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  test_memgen      - Test ScisSosMem library integration"
	@echo "  run_fcfs         - Run with FCFS scheduler"
	@echo "  run_sjf          - Run with SJF scheduler"
	@echo "  run_priority     - Run with Priority scheduler"
	@echo "  run_rr           - Run with Round Robin scheduler"
	@echo "  experiment1_mem  - Run Experiment 1 with memory manager"
	@echo "  experiment2      - Run Experiment 2 (varying frames)"
	@echo "  run_test_perf    - Run performance tests"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./run_os <scheduler> [experiment] [num_frames]"
	@echo "    scheduler:  fcfs, sjf, priority, rr"
	@echo "    experiment: 1 (default), 2"
	@echo "    num_frames: 32-128 (default: 64)"

.PHONY: all clean run_fcfs run_sjf run_priority run_rr experiment1_nomem experiment1_mem experiment2 run_test_perf test_memgen help
