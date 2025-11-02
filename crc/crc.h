#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stdlib.h>

// CRC32 таблица для быстрых вычислений
extern uint32_t crc32_table[256];

// Инициализация CRC таблицы
void init_crc32_table();

// Вычисление CRC32 для данных
uint32_t crc32(const uint8_t *data, size_t length);

// Интенсивное вычисление CRC с множеством итераций
void intensive_crc_calculation(int iterations, size_t data_size);

#endif