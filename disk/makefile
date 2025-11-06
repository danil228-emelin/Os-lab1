CC = gcc
CFLAGS = -O0 -Wall -Wextra -std=c99
TARGETS = ema_join-sm

all: $(TARGETS)

ema_join-sm: ema_join-sm.c ema_join.c 
	$(CC) $(CFLAGS) -o $@ $^


optimized:ema_join-sm.c ema_join.c 
	$(CC) -O3 -o ema_join-sm-optimed ema_join-sm.c ema_join.c

clean:
	rm -f $(TARGETS) ema_join-sm


.PHONY: all clean optimized

perf-counters: ema_join-sm
	perf stat -e \
	cpu-cycles,instructions,cache-references,cache-misses,\
	page-faults,minor-faults,major-faults,\
	L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores,\
	LLC-loads,LLC-load-misses,\
	dTLB-loads,dTLB-load-misses  ./ema_join-sm  -i 1000 -m 1000 -d


perf-counters-opt: optimized
	perf stat -e \
	cpu-cycles,instructions,cache-references,cache-misses,\
	page-faults,minor-faults,major-faults,\
	L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores,\
	LLC-loads,LLC-load-misses,\
	dTLB-loads,dTLB-load-misses  ./ema_join-sm-optimed


perf-io: ema_join-sm
	perf stat -e \
	duration_time,task-clock,\
	major-faults,context-switches,cpu-migrations,\
	page-faults ./ema_join-sm -i 1000 -m 1000 -d
