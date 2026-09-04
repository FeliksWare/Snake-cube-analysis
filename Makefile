CFLAGS = -std=c99 -pedantic -Wall -Wextra -O3
LDFLAGS =

.PHONY: all clean

all: analyse3x3x3 analyseXYZ

analyse3x3x3: analyse3x3x3.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

analyseXYZ: analyseXYZ.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f analyse3x3x3 analyse3x3x3.o
	rm -f analyseXYZ analyseXYZ.o
