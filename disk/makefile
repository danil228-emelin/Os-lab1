# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
OPT_FLAGS = -O3 -march=native
DEBUG_FLAGS = -O0 -g -DDEBUG

# Targets
TARGET_OPT = ema-join-sm-opt
TARGET_DEBUG = ema-join-sm-debug

# Source files
SRCS = ema-join-sm.c
OBJ_OPT = ema-join-sm-opt.o
OBJ_DEBUG = ema-join-sm-debug.o

# Default target
all: opt debug

# Optimized version
opt: $(TARGET_OPT)

$(TARGET_OPT): $(OBJ_OPT)
	$(CC) $(CFLAGS) $(OPT_FLAGS) -o $@ $^

$(OBJ_OPT): $(SRCS)
	$(CC) $(CFLAGS) $(OPT_FLAGS) -c -o $@ $<

# Debug version (no optimizations)
debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

$(OBJ_DEBUG): $(SRCS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

# Clean
clean:
	rm -f $(TARGET_OPT) $(TARGET_DEBUG) $(OBJ_OPT) $(OBJ_DEBUG)
	rm -f table1.txt table2.txt result_*.txt

#	CPU: cycles, instructions
	
#	Кэш: cache-misses, cache-references

#	Память: page-faults
#perf-opt:perf stat -e cycles,instructions,cache-misses,cache-references,page-faults 


#     Операции ввода-вывода: page-faults, major/minor faults

#	  Производительность чтения/записи

#perf-io:perf stat -e page-faults,minor-faults,major-faults,context-switches
# Phony targets


#	Паттерны доступа к памяти: L1/L2/L3 кэш

#	Эффективность использования кэша
#perf-memory:perf stat -e cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-loads,LLC-load-misses


.PHONY: all opt debug clean