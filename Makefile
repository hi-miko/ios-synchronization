CXX = gcc
CC = $(CXX)
LDLIBS= -pthread -lrt -g
CFLAGS = -Wall -Wextra -pedantic -std=gnu99 -ggdb
VPATH = src ./
EXECUTABLE = proj2
EXTRA = $(EXECUTABLE).out
OBJECTS = proj2.o
DEBUG_VALUES = 1 2 100 100 100

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)

.PHONY: clean val-test

val-test:
	valgrind -s --leak-check=full --show-leak-kinds=all ./$(EXECUTABLE) $(DEBUG_VALUES)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(EXTRA)
