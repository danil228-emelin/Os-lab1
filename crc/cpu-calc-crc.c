#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "crc.h"

void print_usage(const char *program_name) {
    printf("CPU Load Generator - CRC32 Calculator\n");
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -i, --iterations NUMBER  Number of CRC calculation iterations (default: 1000)\n");
    printf("  -s, --size SIZE          Data size in KB (default: 1024 = 1MB)\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("  -h, --help               Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -i 5000 -s 2048      # 5000 iterations with 2MB data\n", program_name);
    printf("  %s --iterations 10000   # 10000 iterations with default 1MB data\n", program_name);
}

int main(int argc, char *argv[]) {
    int iterations = 1000;
    int data_size_kb = 1024; // 1MB по умолчанию
    int verbose = 0;
    
    // Парсинг аргументов командной строки
    static struct option long_options[] = {
        {"iterations", required_argument, 0, 'i'},
        {"size", required_argument, 0, 's'},
        {"threads", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "i:s:t:vh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                iterations = atoi(optarg);
                if (iterations <= 0) {
                    fprintf(stderr, "Error: iterations must be positive\n");
                    return 1;
                }
                break;
                
            case 's':
                data_size_kb = atoi(optarg);
                if (data_size_kb <= 0) {
                    fprintf(stderr, "Error: data size must be positive\n");
                    return 1;
                }
                break;
                
            case 't':
                printf("Threading support coming soon. Using single thread.\n");
                break;
                
            case 'v':
                verbose = 1;
                break;
                
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    if (verbose) {
        printf("=== CPU Load Generator Configuration ===\n");
        printf("Iterations: %d\n", iterations);
        printf("Data size: %d KB (%zu bytes)\n", data_size_kb, (size_t)data_size_kb * 1024);
        printf("Algorithm: CRC32 with lookup table\n");
        printf("=======================================\n");
    }
    
    init_crc32_table();
    
    if (verbose) {
        printf("CRC table initialized\n");
    }
    
    // Запуск интенсивных вычислений
    size_t data_size_bytes = (size_t)data_size_kb * 1024;
    intensive_crc_calculation(iterations, data_size_bytes);
    
    return 0;
}