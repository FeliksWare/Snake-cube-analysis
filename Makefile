CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2
LDFLAGS =

.PHONY: all clean

SRC = main.c
OBJ = ${SRC:.c=.o}

all: main

main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f main $(OBJ)
