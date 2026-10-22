CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Iinclude
TARGET := tests/test_planificador.out
SOURCES := src/planificador.c tests/test_planificador.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) include/planificador.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)
