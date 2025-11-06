#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/shm.h>
#include "ema_join.h"

void print_usage(const char *program_name) {
    printf("Memory/Disk Load Generator - EMA+JOIN with Shared Memory\n");
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -i, --iterations NUMBER  Number of calculation iterations (default: 100)\n");
    printf("  -s, --size SIZE          Data size in thousands of elements (default: 100 = 100K elements)\n");
    printf("  -m, --memory SIZE        Shared memory size in MB (default: 0 = no shared memory)\n");
    printf("  -d, --disk               Enable disk operations (write/read temporary files)\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("  -h, --help               Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -i 500 -s 200           # 500 iterations with 200K elements\n", program_name);
    printf("  %s -i 100 -s 100 -m 10     # With 10MB shared memory\n", program_name);
    printf("  %s -i 50 -s 500 -d         # With disk operations\n", program_name);
}

// Функция для работы с диском (создание временных файлов)
void perform_disk_operations(size_t data_size) {
    printf("Performing disk operations...\n");
    
    FILE *temp_file = tmpfile();
    if (!temp_file) {
        perror("tmpfile");
        return;
    }
    
    // Записываем тестовые данные в файл
    for (size_t i = 0; i < data_size; i++) {
        double value = (double)rand() / RAND_MAX;
        fwrite(&value, sizeof(double), 1, temp_file);
    }
    
    // Читаем данные обратно
    rewind(temp_file);
    double sum = 0.0;
    for (size_t i = 0; i < data_size; i++) {
        double value;
        fread(&value, sizeof(double), 1, temp_file);
        sum += value;
    }
    
    fclose(temp_file);
    printf("Disk operations completed. Data sum: %f\n", sum);
}

int main(int argc, char *argv[]) {
    int iterations = 100;
    int data_size_k = 100; // 100 тысяч элементов по умолчанию
    int shared_memory_mb = 0;
    int disk_operations = 0;
    int verbose = 0;
    
    // Парсинг аргументов командной строки
    static struct option long_options[] = {
        {"iterations", required_argument, 0, 'i'},
        {"size", required_argument, 0, 's'},
        {"memory", required_argument, 0, 'm'},
        {"disk", no_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "i:s:m:dvh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                iterations = atoi(optarg);
                if (iterations <= 0) {
                    fprintf(stderr, "Error: iterations must be positive\n");
                    return 1;
                }
                break;
                
            case 's':
                data_size_k = atoi(optarg);
                if (data_size_k <= 0) {
                    fprintf(stderr, "Error: data size must be positive\n");
                    return 1;
                }
                break;
                
            case 'm':
                shared_memory_mb = atoi(optarg);
                if (shared_memory_mb < 0) {
                    fprintf(stderr, "Error: memory size cannot be negative\n");
                    return 1;
                }
                break;
                
            case 'd':
                disk_operations = 1;
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
        printf("=== Memory/Disk Load Generator Configuration ===\n");
        printf("Iterations: %d\n", iterations);
        printf("Data size: %dK elements (%zu total elements)\n", 
               data_size_k, (size_t)data_size_k * 1000);
        printf("Shared memory: %d MB\n", shared_memory_mb);
        printf("Disk operations: %s\n", disk_operations ? "enabled" : "disabled");
        printf("Algorithms: EMA + JOIN operations\n");
        printf("===============================================\n");
    }
    
    size_t data_size = (size_t)data_size_k * 1000;
    
    // Работа с разделяемой памятью если запрошено
    if (shared_memory_mb > 0) {
        size_t shm_size = (size_t)shared_memory_mb * 1024 * 1024;
        printf("Creating shared memory: %zu bytes\n", shm_size);
        
        int shm_id = create_shared_memory(shm_size);
        if (shm_id != -1) {
            void *shm_ptr = attach_shared_memory(shm_id);
            if (shm_ptr) {
                // Используем разделяемую память для данных
                memset(shm_ptr, 0, shm_size);
                printf("Shared memory initialized and attached\n");
                
                // Здесь можно добавить работу с разделяемой памятью
                cleanup_shared_memory(shm_id, shm_ptr);
            }
        }
    }
    
    // Дисковые операции если запрошено
    if (disk_operations) {
        perform_disk_operations(data_size / 10); // Меньший размер для диска
    }
    
    // Основные вычисления EMA + JOIN
    intensive_ema_join_calculation(iterations, data_size);
    
    if (verbose) {
        printf("All operations completed successfully!\n");
    }
    
    return 0;
}