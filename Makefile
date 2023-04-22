CXX = gcc
CC = $(CXX)
LDLIBS= -pthread
CXXFLAGS = -Wall -Wextra -pedantic -std=gnu99 -g
VPATH = src ./
EXECUTABLE = proj2
OBJECTS = proj2.o

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)

.PHONY: clean

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
