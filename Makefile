# Makefile for ScisSOS

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
LDFLAGS = 

# Directories
OBJ_DIR = obj

# Source files
SOURCES = main.c os.c process.c scheduling_algo.c
TEST_SRC = test_perf.c
OBJECTS = $(addprefix $(OBJ_DIR)/, $(SOURCES:.c=.o))
EXECUTABLE = run_os
TEST_EXECUTABLE = test_perf

# Header files
HEADERS = ScisSos.h scheduling_algo.h

all: $(OBJ_DIR) $(EXECUTABLE)

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

run_fcfs: $(EXECUTABLE)
	./$(EXECUTABLE) fcfs

run_sjf: $(EXECUTABLE)
	./$(EXECUTABLE) sjf

run_priority: $(EXECUTABLE)
	./$(EXECUTABLE) priority

run_rr: $(EXECUTABLE)
	./$(EXECUTABLE) rr

run_test_perf: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

.PHONY: all clean run_fcfs run_sjf run_priority run_rr run_test_perf