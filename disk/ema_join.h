#ifndef EMA_JOIN_H
#define EMA_JOIN_H

#include <stddef.h>

// Структура для данных
typedef struct {
    double timestamp;
    double value;
    int id;
} DataPoint;

// Структура для JOIN операций
typedef struct {
    DataPoint *data1;
    DataPoint *data2;
    size_t size;
    double join_result;
} JoinData;

// Экспоненциальное скользящее среднее
double ema(double previous, double current, double alpha);

// Инициализация данных
void init_data(DataPoint *data, size_t size, int seed);

// JOIN операция между двумя наборами данных
void perform_join(JoinData *join_data);

// Интенсивные вычисления EMA + JOIN
void intensive_ema_join_calculation(int iterations, size_t data_size);

// Работа с разделяемой памятью (Shared Memory)
int create_shared_memory(size_t size);
void* attach_shared_memory(int shm_id);
void cleanup_shared_memory(int shm_id, void *ptr);

#endif