# Makefile for ScisSOS

CC = gcc
CFLAGS = -Wall -Wextra -g 
LDFLAGS = libscismem.a -lm

# Directories
OBJ_DIR = obj

# Source files
SOURCES = main.c os.c process.c scheduling_algo.c memory.c memory_override.c
TEST_SRC = test_perf.c
OBJECTS = $(addprefix $(OBJ_DIR)/, $(SOURCES:.c=.o))
EXECUTABLE = run_os
TEST_EXECUTABLE = test_perf

# Header files
HEADERS = ScisSos.h scheduling_algo.h ScisosMem.h

all: $(OBJ_DIR) $(EXECUTABLE) $(TEST_EXECUTABLE)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(TEST_EXECUTABLE): $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o $(EXECUTABLE) $(TEST_EXECUTABLE)
	rmdir $(OBJ_DIR) 2>/dev/null || true

# Run with memory management enabled (default)
run_fcfs: $(EXECUTABLE)
	./$(EXECUTABLE) sched fcfs

run_sjf: $(EXECUTABLE)
	./$(EXECUTABLE) sched sjf

run_priority: $(EXECUTABLE)
	./$(EXECUTABLE) sched priority

run_rr: $(EXECUTABLE)
	./$(EXECUTABLE) sched rr

# Run without memory management
run_fcfs_nomem: $(EXECUTABLE)
	./$(EXECUTABLE) sched fcfs no-memory

run_sjf_nomem: $(EXECUTABLE)
	./$(EXECUTABLE) sched sjf no-memory

run_priority_nomem: $(EXECUTABLE)
	./$(EXECUTABLE) sched priority no-memory

run_rr_nomem: $(EXECUTABLE)
	./$(EXECUTABLE) sched rr no-memory

# Test performance
run_test_perf: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

# Help target
help:
	@echo "ScisSOS Makefile Targets:"
	@echo "  all              - Build all executables"
	@echo "  clean            - Remove built files"
	@echo ""
	@echo "Run with Memory Management (default):"
	@echo "  run_fcfs         - Run with FCFS scheduler"
	@echo "  run_sjf          - Run with SJF scheduler"
	@echo "  run_priority     - Run with Priority scheduler"
	@echo "  run_rr           - Run with Round Robin scheduler"
	@echo ""
	@echo "Run without Memory Management:"
	@echo "  run_fcfs_nomem   - Run FCFS without memory"
	@echo "  run_sjf_nomem    - Run SJF without memory"
	@echo "  run_priority_nomem - Run Priority without memory"
	@echo "  run_rr_nomem     - Run RR without memory"
	@echo ""
	@echo "Testing:"
	@echo "  run_test_perf    - Run performance tests"

.PHONY: all clean run_fcfs run_sjf run_priority run_rr \
        run_fcfs_nomem run_sjf_nomem run_priority_nomem run_rr_nomem \
        run_test_perf help
