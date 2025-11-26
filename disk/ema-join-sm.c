#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define WORD_SIZE 9  // 8 символов + 1 для '\0'

typedef struct {
    int id;
    char word[WORD_SIZE];
} Row;

typedef struct {
    int size;
    Row *rows;
} Table;

// Функция сравнения для сортировки
int compare_rows(const void *a, const void *b) {
    const Row *row_a = (const Row *)a;
    const Row *row_b = (const Row *)b;
    return row_a->id - row_b->id;
}

// Чтение таблицы из файла
Table read_table(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        exit(1);
    }

    Table table;
    if (fscanf(file, "%d", &table.size) != 1) {
        fprintf(stderr, "Error: Cannot read table size from %s\n", filename);
        fclose(file);
        exit(1);
    }

    table.rows = malloc(table.size * sizeof(Row));
    if (!table.rows) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        exit(1);
    }

    for (int i = 0; i < table.size; i++) {
        if (fscanf(file, "%d %8s", &table.rows[i].id, table.rows[i].word) != 2) {
            fprintf(stderr, "Error: Cannot read row %d from %s\n", i, filename);
            free(table.rows);
            fclose(file);
            exit(1);
        }
    }

    fclose(file);
    return table;
}

// Запись таблицы в файл
void write_table(const char *filename, Table table) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        exit(1);
    }

    fprintf(file, "%d\n", table.size);
    for (int i = 0; i < table.size; i++) {
        fprintf(file, "%d %s\n", table.rows[i].id, table.rows[i].word);
    }

    fclose(file);
}

// Sort-Merge Join алгоритм
Table sort_merge_join(Table table1, Table table2) {
    // Шаг 1: Сортируем обе таблицы по id
    qsort(table1.rows, table1.size, sizeof(Row), compare_rows);
    qsort(table2.rows, table2.size, sizeof(Row), compare_rows);

    // Шаг 2: Подсчитываем размер результата
    int result_size = 0;
    int i = 0, j = 0;
    
    while (i < table1.size && j < table2.size) {
        if (table1.rows[i].id < table2.rows[j].id) {
            i++;
        } else if (table1.rows[i].id > table2.rows[j].id) {
            j++;
        } else {
            // Найдены совпадающие id
            int current_id = table1.rows[i].id;
            int count1 = 0, count2 = 0;
            
            // Подсчитываем количество строк с current_id в table1
            int temp_i = i;
            while (temp_i < table1.size && table1.rows[temp_i].id == current_id) {
                count1++;
                temp_i++;
            }
            
            // Подсчитываем количество строк с current_id в table2
            int temp_j = j;
            while (temp_j < table2.size && table2.rows[temp_j].id == current_id) {
                count2++;
                temp_j++;
            }
            
            result_size += count1 * count2;
            i = temp_i;
            j = temp_j;
        }
    }

    // Шаг 3: Выполняем join и создаем результирующую таблицу
    Table result;
    result.size = result_size;
    result.rows = malloc(result_size * sizeof(Row));
    if (!result.rows) {
        fprintf(stderr, "Error: Memory allocation failed for result\n");
        exit(1);
    }

    int result_index = 0;
    i = 0;
    j = 0;

    while (i < table1.size && j < table2.size) {
        if (table1.rows[i].id < table2.rows[j].id) {
            i++;
        } else if (table1.rows[i].id > table2.rows[j].id) {
            j++;
        } else {
            int current_id = table1.rows[i].id;
            
            // Находим границы блоков с одинаковым id в обеих таблицах
            int start_i = i;
            int start_j = j;
            
            while (i < table1.size && table1.rows[i].id == current_id) {
                i++;
            }
            while (j < table2.size && table2.rows[j].id == current_id) {
                j++;
            }
            
            // Выполняем декартово произведение блоков
            for (int k = start_i; k < i; k++) {
                for (int l = start_j; l < j; l++) {
                    result.rows[result_index].id = table1.rows[k].id;
                    // Формируем объединенное слово (можно модифицировать по необходимости)
                    snprintf(result.rows[result_index].word, WORD_SIZE, "%s", 
                             table1.rows[k].word); // или комбинировать слова
                    result_index++;
                }
            }
        }
    }

    return result;
}

// Генерация тестовых данных
void generate_test_data(const char *filename, int size) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        exit(1);
    }

    fprintf(file, "%d\n", size);
    
    const char *words[] = {
        "apple", "banana", "cherry", "date", "elder", "fig", "grape", "honey",
        "ice", "juice", "kiwi", "lemon", "mango", "nut", "orange", "pear"
    };
    int words_count = sizeof(words) / sizeof(words[0]);

    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        int id = rand() % (size / 2 + 1); // Генерируем повторяющиеся id
        const char *word = words[rand() % words_count];
        fprintf(file, "%d %s\n", id, word);
    }

    fclose(file);
    printf("Generated test data: %s with %d rows\n", filename, size);
}

// Освобождение памяти таблицы
void free_table(Table table) {
    free(table.rows);
}

int main(int argc, char *argv[]) {
    if (argc != 4 && argc != 2) {
        printf("Usage:\n");
        printf("  %s <table1_file> <table2_file> <output_file>\n", argv[0]);
        printf("  %s --generate <size1> <size2>\n", argv[0]);
        printf("Example: %s table1.txt table2.txt result.txt\n", argv[0]);
        printf("         %s --generate 1000 500\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--generate") == 0) {
        if (argc != 4) {
            printf("Usage: %s --generate <size1> <size2>\n", argv[0]);
            return 1;
        }
        
        int size1 = atoi(argv[2]);
        int size2 = atoi(argv[3]);
        
        generate_test_data("table1.txt", size1);
        generate_test_data("table2.txt", size2);
        
        printf("Test files generated: table1.txt (%d rows), table2.txt (%d rows)\n", size1, size2);
        return 0;
    }

    // Измеряем время выполнения
    clock_t start_time = clock();

    // Чтение входных таблиц
    Table table1 = read_table(argv[1]);
    Table table2 = read_table(argv[2]);

    printf("Table1: %d rows\n", table1.size);
    printf("Table2: %d rows\n", table2.size);

    // Выполнение Sort-Merge Join
    Table result = sort_merge_join(table1, table2);

    // Запись результата
    write_table(argv[3], result);

    // Освобождение памяти
    free_table(table1);
    free_table(table2);
    free_table(result);

    clock_t end_time = clock();
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Join completed successfully!\n");
    printf("Result: %d rows written to %s\n", result.size, argv[3]);
    printf("Execution time: %.3f seconds\n", execution_time);

    return 0;
}