#!/bin/bash

# Script for comprehensive performance testing of optimized and debug versions
# Usage: ./perf_test.sh <table1_file> <table2_file> <output_prefix>

if [ $# -ne 3 ]; then
    echo "Usage: $0 <table1_file> <table2_file> <output_prefix>"
    echo "Example: $0 table1.txt table2.txt result"
    exit 1
fi

TABLE1=$1
TABLE2=$2
OUTPUT_PREFIX=$3

OPT_TARGET="./ema-join-sm-opt"
DEBUG_TARGET="./ema-join-sm-debug"

# Check if binaries exist
if [ ! -f "$OPT_TARGET" ]; then
    echo "Error: Optimized binary $OPT_TARGET not found. Run 'make opt' first."
    exit 1
fi

if [ ! -f "$DEBUG_TARGET" ]; then
    echo "Error: Debug binary $DEBUG_TARGET not found. Run 'make debug' first."
    exit 1
fi

# Check if input files exist
if [ ! -f "$TABLE1" ]; then
    echo "Error: Input file $TABLE1 not found."
    exit 1
fi

if [ ! -f "$TABLE2" ]; then
    echo "Error: Input file $TABLE2 not found."
    exit 1
fi

echo "=================================================="
echo "PERFORMANCE TEST SCRIPT"
echo "Input files: $TABLE1, $TABLE2"
echo "Output prefix: $OUTPUT_PREFIX"
echo "=================================================="
echo

# Function to print separator
print_separator() {
    echo "--------------------------------------------------"
    echo "$1"
    echo "--------------------------------------------------"
}

# Function to run perf test and display results
run_perf_test() {
    local test_name=$1
    local perf_events=$2
    local target_binary=$3
    local output_file=$4
    local version=$5
    
    print_separator "$test_name - $version"
    
    echo "Command: perf stat -e $perf_events $target_binary $TABLE1 $TABLE2 ${output_file}_${version}.txt"
    echo
    
    # Run perf stat and capture output
    perf stat -e $perf_events $target_binary $TABLE1 $TABLE2 ${output_file}_${version}.txt 2>&1 | \
    while IFS= read -r line; do
        if [[ $line == *"seconds time elapsed"* ]] || \
           [[ $line == *"cycles"* ]] || \
           [[ $line == *"instructions"* ]] || \
           [[ $line == *"cache-misses"* ]] || \
           [[ $line == *"cache-references"* ]] || \
           [[ $line == *"page-faults"* ]] || \
           [[ $line == *"major-faults"* ]] || \
           [[ $line == *"minor-faults"* ]] || \
           [[ $line == *"context-switches"* ]] || \
           [[ $line == *"L1-dcache"* ]] || \
           [[ $line == *"LLC-load"* ]]; then
            echo "  $line"
        fi
    done
    
    # Verify output file was created
    if [ -f "${output_file}_${version}.txt" ]; then
        local result_size=$(head -n1 "${output_file}_${version}.txt")
        echo "  Result: ${result_size} rows written to ${output_file}_${version}.txt"
    else
        echo "  ERROR: Output file was not created!"
    fi
    
    echo
}

# Test 1: CPU and Basic Performance
print_separator "TEST 1: CPU PERFORMANCE (cycles, instructions)"
echo "Testing CPU efficiency and instruction count"
echo

run_perf_test "CPU Performance" "cycles,instructions,cache-misses,cache-references,page-faults" \
    "$OPT_TARGET" "${OUTPUT_PREFIX}_cpu" "opt"

run_perf_test "CPU Performance" "cycles,instructions,cache-misses,cache-references,page-faults" \
    "$DEBUG_TARGET" "${OUTPUT_PREFIX}_cpu" "debug"

echo
echo

# Test 2: IO Performance
print_separator "TEST 2: IO PERFORMANCE (page faults, context switches)"
echo "Testing Input/Output operations and memory management"
echo

run_perf_test "IO Performance" "page-faults,minor-faults,major-faults,context-switches" \
    "$OPT_TARGET" "${OUTPUT_PREFIX}_io" "opt"

run_perf_test "IO Performance" "page-faults,minor-faults,major-faults,context-switches" \
    "$DEBUG_TARGET" "${OUTPUT_PREFIX}_io" "debug"

echo
echo

# Test 3: Memory Access Patterns
print_separator "TEST 3: MEMORY ACCESS PATTERNS (cache efficiency)"
echo "Testing memory hierarchy and cache performance"
echo

run_perf_test "Memory Access" "cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-loads,LLC-load-misses" \
    "$OPT_TARGET" "${OUTPUT_PREFIX}_memory" "opt"

run_perf_test "Memory Access" "cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-loads,LLC-load-misses" \
    "$DEBUG_TARGET" "${OUTPUT_PREFIX}_memory" "debug"

echo
echo

# Additional Test: Simple time comparison
print_separator "TEST 4: EXECUTION TIME COMPARISON"
echo "Simple execution time measurement"
echo

echo "Optimized version:"
time $OPT_TARGET $TABLE1 $TABLE2 ${OUTPUT_PREFIX}_time_opt.txt > /dev/null 2>&1
echo

echo "Debug version:"
time $DEBUG_TARGET $TABLE1 $TABLE2 ${OUTPUT_PREFIX}_time_debug.txt > /dev/null 2>&1
echo

# Summary
print_separator "TEST SUMMARY"
echo "All performance tests completed!"
echo "Output files created with prefix: $OUTPUT_PREFIX"
echo
echo "Generated files:"
ls -la ${OUTPUT_PREFIX}_* 2>/dev/null | while read file; do
    if [ -f "$file" ]; then
        size=$(head -n1 "$file" 2>/dev/null || echo "unknown")
        echo "  $file - $size rows"
    fi
done

echo
echo "=================================================="
echo "PERFORMANCE TESTING COMPLETED"
echo "=================================================="