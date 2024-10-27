#include <iostream> 
#include <string>
#include "structures.h"
#include <fstream>
#include <cstdint>

using namespace std;

// Функция для загрузки содержимого JSON файла в память
char* LoadJsonFile(const char* fname, int64_t& size) {
    // Открываем файл в бинарном режиме и перемещаемся в конец, чтобы получить размер файла
    ifstream file(fname, ios::binary | ios::ate); //считывание без обработки данных + перемещаем указатель на конец файла

    // Проверяем, удалось ли открыть файл
    if (!file) {
        cerr << "Ошибка: не удалось открыть файл." << endl;
        exit(1);
    }

    // Получаем размер файла
    size = file.tellg(); //возвращаем текущее положение указателя в файле
    file.seekg(0, ios::beg); // Возвращаем указатель относительно начала

    // Выделяем память под содержимое файла + 1 для завершающего нулевого символа
    char* content = new char[size + 1];

    // Читаем содержимое файла в память
    if (!file.read(content, size)) {
        cerr << "Ошибка: не удалось прочитать файл." << endl;
        delete[] content; // Освобождаем память в случае ошибки
        exit(1);
    }

    // Добавляем завершающий нулевой символ для корректной работы с C-строкой
    content[size] = '\0';

    // Закрываем файл
    file.close();

    return content;
}

// Функция для извлечения строки, заключённой в кавычки, из содержимого файла
string take_string(char* cont, int& index) {
    string content;
    bool get_content = false; // Флаг завершения извлечения строки
    bool getting_content = false; // Флаг начала извлечения строки

    while (true) {
        if (get_content) {
            break; // Завершаем извлечение при нахождении второй кавычки
        } else if (!get_content) {
            if (cont[index] == '"') { // Проверяем на кавычки
                if (getting_content){
                    get_content = true; // Завершение строки
                } else {
                    getting_content = true; // Начало строки
                }
            } else if (getting_content) {
                content.push_back(cont[index]); // Сохраняем символы строки
            }    
        }
        index++; // Переход к следующему символу
    }
    return content;
}

// Функция для извлечения списка, заключённого в квадратные скобки, из содержимого файла
string take_list(char* cont, int& index) {
    string content;
    bool get_content = false;
    bool getting_content = false;

    while (true) {
        if (get_content){ 
            break; // Завершение извлечения при нахождении закрывающей скобки
        } else if (!get_content) {
            if (cont[index] == '[' || cont[index] == ']') {
                if (getting_content){ 
                    get_content = true; // Завершение списка
                } else {
                    getting_content = true; // Начало списка
                }
            } else if (getting_content) { 
                content.push_back(cont[index]); // Сохраняем элементы списка
            }
        }

        index++; // Переход к следующему символу
    }
    return content;
}

// Функция для извлечения целого числа из содержимого файла
int take_int(char* cont, int& index) {
    int content = 0;

    bool get_content = false;
    bool getting_content = false;
    bool got_at_least_one = false; // Флаг, что было извлечено хотя бы одно число

    while (true) {
        if (get_content){ 
            break; // Завершаем извлечение при нахождении нецифрового символа после числа
        }
        if (!get_content) {
            if ('0' > cont[index] || '9' < cont[index]) {
                if (getting_content && got_at_least_one){ 
                    get_content = true; // Завершение числа
                } else {
                    getting_content = true; // Начало числа
                }
            } else if (getting_content) {
                got_at_least_one = true;
                content = content * 10 + (cont[index] - '0'); // Формируем число
            }
        }

        index++; // Переход к следующему символу
    }
    return content;
}

// Функция для конвертации строки в список строк (разделение по кавычкам)
List<string> convert_string(string str) {
    List<string> res;
    string name;

    bool getting = false; // Флаг нахождения в строке

    int index = 0;
    while (index < str.length()) {
        if (str[index] == '"') {
            if (!getting){ 
                getting = true; // Начало новой строки
            } else {
                getting = false; // Конец строки
                res.push(name);
                name = "";
            }
        } else if (getting) {
            name.push_back(str[index]); // Добавляем символы строки
        }
        index++;
    }

    return res;
}

// Функция для извлечения ключ-значений из JSON файла
void take_pairs(char* cont, int& index, int size, HashTable<List<string>>& tables, List<string>& tables_names, string& schem_name, int& tuples_limit) {
    string name;
    string content;
    string table_name;
    string columns;

    bool get_name = false;
    bool getting_name = false;

    // Основной цикл по содержимому JSON файла
    while (index <= size) {
        if (!get_name) {
            if (cont[index] == '"') {
                if (getting_name){ 
                    get_name = true; // Завершение извлечения имени
                } else {
                    getting_name = true; // Начало имени
                }
            } else if (getting_name) {
                name.push_back(cont[index]); // Добавляем символы имени
            }
        } else {
            // Обработка различных параметров в JSON: схема, лимит кортежей и структура
            if (name == "name") { 
                schem_name = take_string(cont, index); // Извлекаем имя схемы
            } else if (name == "tuples_limit"){
                 tuples_limit = take_int(cont, index); // Извлекаем лимит кортежей
            } else if (name == "structure") {
                // Извлекаем таблицы и их структуру
                while (cont[index] != '}') {
                    table_name = take_string(cont, index); // Имя таблицы
                    columns = take_list(cont, index); // Столбцы таблицы
                    List<string> columns_li = convert_string(columns);
                    tables_names.push(table_name); // Добавляем имя таблицы в список
                    tables.Add(table_name, columns_li); // Добавляем таблицу в хеш-таблицу
                    while (cont[index] != '"' && cont[index] != '}') {
                        index++;
                    }
                }
            }
            get_name = false;
            getting_name = false;
            name = ""; // Очищаем переменную для следующего ключа
        }
        index++;
    }
}

// Функция для загрузки JSON файла и извлечения данных
void getJSON(HashTable<List<string>>& tables, List<string>& tables_names, string& schem_name, int& limit) {
    int64_t size;
    char* data = LoadJsonFile("shema.json", size); // Загружаем содержимое файла

    int i = 0;

    // Извлекаем ключ-значения и добавляем их в структуры
    take_pairs(data, i, size, tables, tables_names, schem_name, limit);

    // Освобождаем память после обработки файла
    delete[] data;
}
