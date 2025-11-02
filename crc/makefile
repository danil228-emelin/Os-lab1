CC = gcc
CFLAGS = -O0 -Wall -Wextra -std=c99
TARGETS = cpu-calc-crc

all: $(TARGETS)

cpu-calc-crc: cpu-calc-crc.c crc.c
	$(CC) $(CFLAGS) -o $@ $^


optimized: cpu-calc-crc.c crc.c
	$(CC) -O3 -o cpu-calc-crc-opt cpu-calc-crc.c crc.c

clean:
	rm -f $(TARGETS) cpu-calc-crc-opt

count-crc: cpu-calc-crc
	perf stat -d ./cpu-calc-crc


.PHONY: all clean optimized count-crc