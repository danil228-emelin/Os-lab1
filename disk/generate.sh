#!/bin/bash

# Генерация файлов с 1000 строками

generate_table_1000() {
    local filename=$1
    local version=$2
    
    echo "1000" > "$filename"
    
    case $version in
        "type1")
            # Первый вид: последовательные ID с дубликатами
            for ((i=1; i<=1000; i++)); do
                local id=$(( (i % 200) + 1 ))  # ID от 1 до 200 для дубликатов
                local words=("apple" "banana" "cherry" "date" "elder" "fig" "grape" "honey" "ice" "juice" "kiwi" "lemon" "mango" "nut" "orange" "pear")
                local word=${words[$((RANDOM % ${#words[@]}))]}
                echo "$id $word" >> "$filename"
            done
            ;;
        "type2")
            # Второй вид: случайные ID с большим разбросом
            for ((i=1; i<=1000; i++)); do
                local id=$((RANDOM % 500 + 1))  # ID от 1 до 500
                local words=("alpha" "bravo" "charlie" "delta" "echo" "foxtrot" "golf" "hotel" "india" "juliet" "kilo" "lima" "mike" "november" "oscar" "papa")
                local word=${words[$((RANDOM % ${#words[@]}))]}
                echo "$id $word" >> "$filename"
            done
            ;;
    esac
    
    echo "Generated $filename ($version)"
}

# Генерация файлов
echo "Generating 1000-row tables..."

# Первый вид таблиц
generate_table_1000 "table1_1000_type1.txt" "type1"
generate_table_1000 "table2_1000_type1.txt" "type1"

# Второй вид таблиц  
generate_table_1000 "table1_1000_type2.txt" "type2"
generate_table_1000 "table2_1000_type2.txt" "type2"

echo "All 1000-row tables generated!"
echo "Files created:"
echo "  table1_1000_type1.txt - Type 1 (sequential IDs with duplicates)"
echo "  table2_1000_type1.txt - Type 1 (sequential IDs with duplicates)" 
echo "  table1_1000_type2.txt - Type 2 (random IDs with spread)"
echo "  table2_1000_type2.txt - Type 2 (random IDs with spread)"