#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include "structures.h"

// Функция блокировки таблицы. Создает файл с именем tableName + "_lock" и записывает туда значение 0.
void lock(const filesystem::path& directory, const string tableName) {
    filesystem::path filePath = directory / (tableName + "_lock");

    ofstream file(filePath, ios::out | ios::trunc);  // Открываем файл для записи, очищаем содержимое
    file << 0;  // Записываем значение 0, чтобы показать, что таблица заблокирована
    file.close();  // Закрываем файл
}

// Функция разблокировки таблицы. Создает файл с именем tableName + "_lock" и записывает туда значение 1.
void unlock(const filesystem::path& directory, const string tableName) {
    filesystem::path filePath = directory / (tableName + "_lock");

    ofstream file(filePath, ios::out | ios::trunc);  // Открываем файл для записи, очищаем содержимое
    file << 1;  // Записываем значение 1, чтобы показать, что таблица разблокирована
    file.close();  // Закрываем файл
}

// Функция проверки, разблокирована ли таблица. Возвращает 1, если таблица разблокирована, 0 — если заблокирована.
int isunlocked(const filesystem::path& directory, const string& tableName) {
    filesystem::path filePath = directory / (tableName + "_lock");

    ifstream file(filePath);  // Открываем файл для чтения
    int value = 1;  // Значение по умолчанию — разблокировано
    file >> value;  // Читаем содержимое файла (0 или 1)
    file.close();  // Закрываем файл

    return value;  // Возвращаем состояние блокировки
}

// Функция записи данных в CSV-файл. 
void writeToCsv(const filesystem::path& filename, List<List<string>>& data, string table_name, string& schem_name) {
    filesystem::path filePath = ".";  
    // Проверяем, разблокирована ли таблица. Если нет, ждем.
    while (isunlocked(filePath / schem_name / table_name, table_name) == 0) {
        cout << "Waiting for unlock" << endl;
    }

    // Блокируем таблицу перед записью
    lock(filePath / schem_name / table_name, table_name);
    
    ofstream file(filename, ios::out | ios::trunc);  // Открываем файл для записи (очищаем его)

    // Записываем данные в CSV-формате
    for (int i = 0; i < data.length; i++) {
        for (size_t j = 0; j < data[i].length; j++) {
            file << data[i][j];  // Записываем элемент строки
            if (j < data[i].length - 1) {
                file << ",";  // Если не последний элемент, добавляем запятую
            }
        }
        file << "\n";  // Переход на новую строку
    }

    file.close();  // Закрываем файл
    unlock(filePath / schem_name / table_name, table_name);  // Разблокируем таблицу после записи
}

// Функция чтения данных из CSV-файла.
void readFromCsv(const filesystem::path& filename, List<List<string>>& data, string table_name, string& schem_name) {
    filesystem::path filePath = ".";  
    // Проверяем, разблокирована ли таблица. Если нет, ждем.
    while (!isunlocked(filePath / schem_name / table_name, table_name)) {
        cout << "Waiting for unlock" << endl;
    }

    // Блокируем таблицу перед чтением
    lock(filePath / schem_name / table_name, table_name);

    ifstream file(filename);  // Открываем файл для чтения
    string line;

    // Читаем файл построчно
    while (getline(file, line)) {
        List<string> row;
        string cell;
        istringstream lineStream(line);

        // Разделяем строку на ячейки по запятым
        while (getline(lineStream, cell, ',')) {
            row.push(cell);  // Добавляем ячейку в строку
        }

        data.push(row);  // Добавляем строку в список строк (таблицу)
    }

    file.close();  // Закрываем файл
    unlock(filePath / schem_name / table_name, table_name);  // Разблокируем таблицу после чтения
}

// Функция добавления новой строки в CSV-файл.
void appendToCsv(const filesystem::path& filename, List<string>& newRow, string table_name, string& schem_name) {
    filesystem::path filePath = ".";  
    // Проверяем, разблокирована ли таблица. Если нет, ждем.
    while (!isunlocked(filePath / schem_name / table_name, table_name)) {
        cout << "Waiting for unlock" << endl;
    }

    // Блокируем таблицу перед добавлением данных
    lock(filePath / schem_name / table_name, table_name);

    ofstream file(filename, ios::app);  // Открываем файл для добавления (режим append)

    // Записываем новую строку в CSV-формате
    for (size_t i = 0; i < newRow.length; ++i) {
        file << newRow[i];  // Записываем элемент строки
        if (i < newRow.length - 1) {
            file << ",";  // Если не последний элемент, добавляем запятую
        }
    }
    file << "\n";  // Переход на новую строку

    file.close();  // Закрываем файл
    unlock(filePath / schem_name / table_name, table_name);  // Разблокируем таблицу после добавления
}
