#!/bin/bash

echo "======================================================"
echo "           DETAILED IO STATISTICS WITH TIMING"
echo "======================================================"
echo "Program: $@"
echo "Start time: $(date)"
echo "======================================================"

# Запуск с perf
perf stat -e \
duration_time,task-clock,cpu-cycles,instructions,\
major-faults,minor-faults,page-faults,\
block:block_rq_issue,block:block_rq_complete,\
block:block_rq_insert,block:block_rq_abort,\
context-switches,cpu-migrations \
"$@"

echo "======================================================"
echo "End time: $(date)"
echo "======================================================"