#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint32_t crc32_table[256];

// Инициализация таблицы CRC32
void init_crc32_table() {
    uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) {
                c = polynomial ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
}

// Быстрое вычисление CRC32 с использованием таблицы
uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        uint32_t table_index = (crc ^ byte) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[table_index];
    }
    
    return ~crc;
}

// Интенсивные вычисления CRC
void intensive_crc_calculation(int iterations, size_t data_size) {
    printf("Starting CRC calculations...\n");
    printf("Iterations: %d, Data size: %zu bytes\n", iterations, data_size);
    
    // Выделяем память для тестовых данных
    uint8_t *test_data = malloc(data_size);
    if (!test_data) {
        fprintf(stderr, "Memory allocation failed!\n");
        return;
    }
    
    // Заполняем случайными данными
    srand(time(NULL));
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = rand() % 256;
    }
    
    uint32_t final_result = 0;
    
    // Основной цикл вычислений
    for (int i = 0; i < iterations; i++) {
        // Меняем немного данные для разнообразия вычислений
        if (i % 100 == 0) {
            test_data[rand() % data_size] = rand() % 256;
        }
        
        // Вычисляем CRC
        uint32_t result = crc32(test_data, data_size);
        
        // Используем результат чтобы компилятор не оптимизировал вычисления
        final_result ^= result;
        
        // Прогресс для длительных вычислений
        if (iterations > 1000 && i % (iterations / 10) == 0) {
            printf("Progress: %d%%\n", (i * 100) / iterations);
        }
    }
    
    printf("Final XOR result: 0x%08X\n", final_result);
    printf("CRC calculations completed!\n");
    
    free(test_data);
}