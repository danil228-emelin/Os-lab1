#!/bin/bash

echo "=== Тестирование нагрузки CRC32 ==="

# Проверка бинарника
if [ ! -f "./cpu-calc-crc" ]; then
    echo "Ошибка: файл cpu-calc-crc не найден"
    echo "Сначала выполните: make cpu-calc-crc"
    exit 1
fi

# Ввод количества инстансов
while true; do
    read -p "Введите количество инстансов для запуска: " instances
    if [[ "$instances" =~ ^[0-9]+$ ]] && [ "$instances" -ge 1 ]; then
        break
    else
        echo "Ошибка: введите положительное число"
    fi
done

echo "=== Запуск $instances инстансов ==="

start_time=$(date +%s%N)

# Массив для хранения PID'ов
pids=()

# Запуск инстансов
for ((i=1; i<=instances; i++)); do
    echo "Запуск инстанса $i..."
    ./cpu-calc-crc 5000 1048576 $i &
    pids+=($!)
    echo "Инстанс $i запущен с PID: ${pids[-1]}"
done

echo "Все инстансы запущены, ожидание завершения..."

# Ожидание завершения всех процессов
for pid in "${pids[@]}"; do
    wait $pid
    status=$?
    if [ $status -eq 0 ]; then
        echo "Процесс $pid завершен успешно"
    else
        echo "Процесс $pid завершен с ошибкой: $status"
    fi
done

end_time=$(date +%s%N)
elapsed=$(( (end_time - start_time) / 1000000 ))

echo "=== Результаты ==="
echo "Количество инстансов: $instances"
echo "Общее время выполнения: ${elapsed}ms"
echo "Среднее время на инстанс: $((elapsed / instances))ms"