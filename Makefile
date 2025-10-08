# Makefile for ScisSOS

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
LDFLAGS = 

# Source files
SOURCES = main.c os.c process.c scheduling_algo.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = run_os

# Header files
HEADERS = ScisSos.h scheduling_algo.h

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run_fcfs: $(EXECUTABLE)
	./$(EXECUTABLE) fcfs

run_sjf: $(EXECUTABLE)
	./$(EXECUTABLE) sjf

run_priority: $(EXECUTABLE)
	./$(EXECUTABLE) priority

run_rr: $(EXECUTABLE)
	./$(EXECUTABLE) rr

.PHONY: all clean run_fcfs run_sjf run_priority run_rr