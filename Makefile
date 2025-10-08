# Makefile for ScisSOS

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
LDFLAGS = 

# Source files
SOURCES = main.c os.c process.c scheduling_algo.c
TEST_SRC = test_perf.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = run_os
TEST_EXECUTABLE = test_perf

# Header files
HEADERS = ScisSos.h scheduling_algo.h

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(TEST_EXECUTABLE): $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(TEST_EXECUTABLE)

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
