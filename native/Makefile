CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Iinclude
TARGET := tests/test_planificador.out
SOURCES := src/planificador.c tests/test_planificador.c
LIBRARY := libplanificador.so
LIB_SOURCES := src/planificador.c

.PHONY: all library test clean

all: $(TARGET)

$(TARGET): $(SOURCES) include/planificador.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(LIBRARY): $(LIB_SOURCES) include/planificador.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(LIB_SOURCES) -o $(LIBRARY)

library: $(LIBRARY)

test: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(LIBRARY)
