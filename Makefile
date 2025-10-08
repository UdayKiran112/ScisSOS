# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Executable name
TARGET = run_os

# Source files
SRCS = main.c os.c process.c scheduling_algo.c

# Object files in obj/ folder
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files to obj/%.o
obj/%.o: %.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj folder if it doesn't exist
obj:
	mkdir -p obj

# Clean build files
clean:
	rm -rf obj $(TARGET)

# Phony targets
.PHONY: all clean obj
