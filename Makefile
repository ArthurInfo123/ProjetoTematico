CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude
LIBS    = -lsqlite3
TARGET  = moneyflow

SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all run clean
