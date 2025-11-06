#include "ema_join.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>

// Экспоненциальное скользящее среднее
double ema(double previous, double current, double alpha) {
    return alpha * current + (1.0 - alpha) * previous;
}

// Инициализация тестовых данных
void init_data(DataPoint *data, size_t size, int seed) {
    srand(seed);
    
    for (size_t i = 0; i < size; i++) {
        data[i].timestamp = (double)i;
        data[i].value = (double)rand() / RAND_MAX * 100.0; // Значения от 0 до 100
        data[i].id = rand() % 1000;
    }
}

// JOIN операция (имитация SQL JOIN)
void perform_join(JoinData *join_data) {
    double sum = 0.0;
    int match_count = 0;
    
    // "JOIN" по полю id - находим совпадающие записи
    for (size_t i = 0; i < join_data->size; i++) {
        for (size_t j = 0; j < join_data->size; j++) {
            if (join_data->data1[i].id == join_data->data2[j].id) {
                // Вычисляем произведение значений для совпавших записей
                double product = join_data->data1[i].value * join_data->data2[j].value;
                sum += product;
                match_count++;
                
                // Ограничиваем глубину JOIN для производительности
                if (match_count > 1000) break;
            }
        }
    }
    
    join_data->join_result = (match_count > 0) ? sum / match_count : 0.0;
}

// Интенсивные вычисления EMA + JOIN
void intensive_ema_join_calculation(int iterations, size_t data_size) {
    printf("Starting EMA+JOIN calculations...\n");
    printf("Iterations: %d, Data size: %zu elements\n", iterations, data_size);
    
    // Выделяем память для данных
    DataPoint *data1 = malloc(data_size * sizeof(DataPoint));
    DataPoint *data2 = malloc(data_size * sizeof(DataPoint));
    
    if (!data1 || !data2) {
        fprintf(stderr, "Memory allocation failed!\n");
        return;
    }
    
    // Инициализируем данные
    init_data(data1, data_size, 1);
    init_data(data2, data_size, 2);
    
    JoinData join_data = {
        .data1 = data1,
        .data2 = data2,
        .size = data_size,
        .join_result = 0.0
    };
    
    double ema_result = 0.0;
    const double alpha = 0.1;
    
    // Основной цикл вычислений
    for (int i = 0; i < iterations; i++) {
        // Периодически обновляем данные для разнообразия
        if (i % 50 == 0) {
            init_data(data1, data_size, i + 1);
        }
        
        // Выполняем JOIN операцию
        perform_join(&join_data);
        
        // Применяем EMA к результату JOIN
        ema_result = ema(ema_result, join_data.join_result, alpha);
        
        // Прогресс для длительных вычислений
        if (iterations > 100 && i % (iterations / 10) == 0) {
            printf("Progress: %d%%, Current EMA: %f\n", 
                   (i * 100) / iterations, ema_result);
        }
    }
    
    printf("Final EMA result: %f\n", ema_result);
    printf("Final JOIN result: %f\n", join_data.join_result);
    printf("EMA+JOIN calculations completed!\n");
    
    free(data1);
    free(data2);
}

// Работа с разделяемой памятью
int create_shared_memory(size_t size) {
    key_t key = ftok("/tmp", 'E');
    if (key == -1) {
        perror("ftok");
        return -1;
    }
    
    int shm_id = shmget(key, size, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return -1;
    }
    
    return shm_id;
}

void* attach_shared_memory(int shm_id) {
    void *ptr = shmat(shm_id, NULL, 0);
    if (ptr == (void*) -1) {
        perror("shmat");
        return NULL;
    }
    
    return ptr;
}

void cleanup_shared_memory(int shm_id, void *ptr) {
    if (shmdt(ptr) == -1) {
        perror("shmdt");
    }
    
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
}