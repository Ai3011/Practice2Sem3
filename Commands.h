#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "structures.h"
#include "json_parse.h"
#include "csv.h"

using namespace std;

// Функция для вывода меню с описанием синтаксиса команд
void showMenu() {
    cout << "Доступные команды:" << endl;
    cout << "1. SELECT: Выполняет выборку данных." << endl;
    cout << "   Синтаксис: SELECT <table_name.column_name, ...> FROM <table_name, ...> [WHERE condition] [AND/OR condition]" << endl;
    cout << endl;

    cout << "2. INSERT: Вставляет новые данные в таблицу." << endl;
    cout << "   Синтаксис: INSERT INTO <table-name> VALUES ('value1', 'value2', ...);" << endl;
    cout << endl;

    cout << "3. DELETE: Удаляет данные из таблицы." << endl;
    cout << "   Синтаксис: DELETE FROM <table_name> [WHERE condition] [AND/OR condition];" << endl;
    cout << endl;

    cout << "4. END: Завершает работу программы." << endl;
    cout << endl;
}

// Реализация функции установки первичного ключа
void setPrimaryKeyFile(const filesystem::path& directory, const std::string& tableName, int key) {
    filesystem::path filePath = directory / (tableName + "_pk"); // Путь к файлу с ключом
    ofstream file(filePath, ios::out | ios::trunc); // Открытие файла для записи
    file << key; // Запись ключа
    file.close();
}

// Реализация функции чтения первичного ключа
int readPrimaryKeyValue(const filesystem::path& directory, const std::string& tableName) {
    filesystem::path filePath = directory / (tableName + "_pk"); // Путь к файлу с ключом
    ifstream file(filePath); // Открытие файла для чтения
    int value;
    file >> value; // Чтение значения ключа
    file.close();
    return value; // Возврат ключа
}
// Функция для проверки наличия CSV файлов и создания их при отсутствии
void check_csv(string& schem_name, HashTable<List<string>>& tables, List<string>& tables_names) {
    // Устанавливаем путь к директории схемы
    filesystem::path dirPath = ".";
    dirPath = dirPath / schem_name;

    // Проверяем, существует ли директория схемы
    if (filesystem::exists(dirPath) & filesystem::is_directory(dirPath)) {
        return; // Если существует, выходим из функции
    } else {
        // Если директория не существует, создаем директории для каждой таблицы
        for (int i = 0; i < tables_names.length; i++) {
            // Создаем директорию для таблицы
            filesystem::create_directories(dirPath / tables_names[i]);
            List<List<string>> headers;

            // Получаем заголовки таблицы и добавляем их в список
            headers.push(tables.Get(tables_names[i]));
            // Разблокируем таблицу
            unlock(dirPath / schem_name / tables_names[i], tables_names[i]);
            // Записываем заголовки в CSV файл
            writeToCsv(dirPath / tables_names[i] / "1.csv", headers, tables_names[i], schem_name);
            // Устанавливаем первичный ключ для таблицы
            setPrimaryKeyFile(dirPath / tables_names[i], tables_names[i], 0);
            headers.clear(); // Очищаем список заголовков
        }
    }
}

// Функция для извлечения имен таблиц и колонок из строки
void get_names(string& line, int& i, List<string>& selected_tables, List<string>& selected_columns) {
    bool more = true; // Переменная для управления циклом

    // Цикл для извлечения имен таблиц и колонок
    while (more) {
        string name; // Переменная для хранения имени таблицы
        string column; // Переменная для хранения имени колонки
        bool geted_name = false; // состояние для отслеживания, извлекаем ли имя или колонку

        // Проверяем, не вышли ли за пределы строки
        if (i > line.length()) {
            throw runtime_error("Wrong syntax"); // Если вышли, выбрасываем ошибку
        }
        
        // Цикл для извлечения имен и колонок
        while (line[i] != ' ' && line[i] != ',' && i < line.length()) {
            if (line[i] == '.') {
                geted_name = true; // Если встретили точку, меняем режим на колонку
                i++;
            }
            if (!geted_name) {
                name.push_back(line[i]); // Собираем имя таблицы
            } else {
                column.push_back(line[i]); // Собираем имя колонки
            }
            i++; // Переходим к следующему символу
        }

        selected_tables.push(name); // Добавляем имя таблицы в список
        selected_columns.push(column); // Добавляем имя колонки в список

        // Сбрасываем переменные для следующего извлечения
        name = "";
        column = "";
        geted_name = false;

        // Проверяем, есть ли еще символы для извлечения
        if (line[i] == ' ' || i >= line.length()) {
            more = false; // Если нет, выходим из цикла
        } else i++; // Иначе продолжаем
        i++; // Переходим к следующему символу
    }
}

// Функция для извлечения имен таблиц из строки
void get_tables(string& line, int& i, List<string>& selected_tables) {
    bool more = true; // Переменная для управления циклом

    // Цикл для извлечения имен таблиц
    while (more) {
        string name; // Переменная для хранения имени таблицы

        // Проверяем, не вышли ли за пределы строки
        if (i > line.length()) {
            throw runtime_error("Wrong syntax"); // Если вышли, выбрасываем ошибку
        }
        
        // Цикл для извлечения имени таблицы
        while (line[i] != ' ' && line[i] != ',' && i < line.length()) {
            name.push_back(line[i]); // Собираем имя таблицы
            i++;
        }

        // Проверяем, не существует ли уже такая таблица
        if (selected_tables.find(name) != -1) {
            throw runtime_error("Wrong syntax"); // Если существует, выбрасываем ошибку
        }
        
        selected_tables.push(name); // Добавляем имя таблицы в список

        name = ""; // Сбрасываем переменную для следующего извлечения
        // Проверяем, есть ли еще символы для извлечения
        if (line[i] == ' ' || i >= line.length()) {
            more = false; // Если нет, выходим из цикла
        } else { 
            i++; // Иначе продолжаем
        }
        i++; // Переходим к следующему символу
    }
}

// Функция для копирования элементов из одного списка в другой
void copyList(List<string>& from, List<string>& to) {
    // Проходим по всем элементам списка from
    for (int i = 0; i < from.length; i++) {
        to.push(from[i]); // Добавляем каждый элемент в список to
    }
}

// Функция для извлечения строки из строки line начиная с индекса i
string take_string(string& line, int& i) {
    string res; // Переменная для хранения результата
    // Извлекаем символы до первого пробела или конца строки
    while (line[i] != ' ' && i < line.length()) {
        res.push_back(line[i]); // Добавляем символ к результату
        i++; // Переходим к следующему символу
    }
    i++; // Пропускаем пробел
    return res; // Возвращаем извлеченную строку
}

// Функция для очистки двойного списка
void double_clear(List<List<string>>& formed) {
    // Проходим по всем подспискам formed
    for (int o = 0; o < formed.length; o++) {
        formed[o].clear(); // Очищаем каждый подсписок
    }
    formed.clear(); // Очищаем основной список
}

// Функция для выполнения перекрестного соединения двух списков
void cross_joint(List<List<string>>& itog, List<List<string>>& pred) {
    // Если предшествующий список пуст, выходим из функции
    if (pred.length == 0) {
        return;
    }

    List<List<string>> res; // Список для хранения результатов

    // Проходим по всем элементам основного списка itog
    for (int i = 0; i < itog.length; i++) {
        // Проходим по всем элементам предшествующего списка pred
        for (int j = 0; j < pred.length; j++) {

            List<string> prom1; // Временный список для хранения промежуточного результата
            List<string> geted = itog[i]; // Получаем текущий элемент itog

            copyList(geted, prom1); // Копируем текущий элемент в prom1

            // Добавляем все элементы из текущего элемента pred в prom1
            for (int z = 0; z < pred[j].length; z++) {
                prom1.push(pred[j][z]);
            }

            res.push(prom1); // Добавляем промышленные результаты в итоговый список
        }
    }

    double_clear(itog); // Очищаем itog перед обновлением
    itog = res; // Обновляем itog новым списком результатов
}
// Функция для проверки синтаксиса, сравнивая выбранные таблицы с таблицами из источника
bool check_synt(List<string>& selected_tables, List<string>& from_tables) {
    // Проходим по всем выбранным таблицам
    for (int i = 0; i < selected_tables.length; i++) {
        // Проверяем, есть ли выбранная таблица в списке источников
        if (from_tables.find(selected_tables[i]) == -1) { 
            return false; // Если нет, возвращаем false
        }
    }
    // Проходим по всем таблицам из источника
    for (int i = 0; i < from_tables.length; i++) {
        // Проверяем, есть ли таблица из источника в списке выбранных
        if (selected_tables.find(from_tables[i]) == -1) { 
            return false; // Если нет, возвращаем false
        }
    }
    return true; // Если все таблицы совпадают, возвращаем true
}

// Функция для печати строк списка to_print
void printRows(List<List<string>>& to_print) {
    // Проходим по всем строкам списка
    for (int i = 0; i < to_print.length; i++) {
        // Проходим по всем элементам текущей строки
        for (int j = 0; j < to_print[i].length; j++) {
            cout << to_print[i][j] << ' '; // Печатаем элемент с пробелом
        }
        cout << endl; // Переходим на новую строку
    }
}

// Функция для печати отфильтрованных строк из списка to_print на основе условия cmp
void printFilteredRows(List<List<string>>& to_print, List<bool>& cmp) {
    // Проходим по всем строкам списка
    for (int i = 0; i < to_print.length; i++) {
        // Если текущая строка не удовлетворяет условию, пропускаем ее
        if (!cmp[i]) {
            continue;
        }
        // Проходим по всем элементам текущей строки
        for (int j = 0; j < to_print[i].length; j++) {
            cout << to_print[i][j] << ' '; // Печатаем элемент с пробелом
        }
        cout << endl; // Переходим на новую строку
    }
}

// Функция для извлечения строки, заключенной в одинарные кавычки
string unwrap_string(string str) {
    string res; // Переменная для хранения результата

    int index = 0; // Индекс для обхода строки
    bool geting = false; // Флаг для отслеживания, находимся ли мы внутри кавычек

    // Проходим по всей строке
    while (index < str.length()) {
        if (str[index] == '\'') { // Проверяем на наличие кавычки
            if (geting) {
                return res; // Если уже внутри кавычек, возвращаем результат
            } else {
                geting = true; // Входим внутрь кавычек
            }
        } else if (geting) { // Если мы внутри кавычек
            res.push_back(str[index]); // Добавляем символ к результату
        }
        index++; // Переходим к следующему символу
    }

    throw runtime_error("Invalid argument"); // Если кавычка не найдена, выбрасываем исключение
}
// Функция для формирования данных на основе заданных таблиц
void form(string& schem_name, List<string>& from_tables, HashTable<List<string>>& rasp, List<string>& posl, List<List<string>>& new_formed, HashTable<List<string>>& tables) {
    filesystem::path dirPath = "."; // Путь к директории

    // Проходим по всем таблицам, из которых будет формироваться результат
    for (int k = 0; k < from_tables.length; k++) {

        List<List<string>> formed; // Список для хранения сформированных данных

        // Проходим по всем полям текущей таблицы
        for (int j = 0; j < rasp.Get(from_tables[k]).length; j++) {
            // Формируем имя поля в формате "таблица.поле"aa
            string to_posl = from_tables[k] + '.' + rasp.Get(from_tables[k])[j];
            posl.push(to_posl); // Добавляем это поле в список полей
            
            List<List<string>> geted; // Список для хранения данных, прочитанных из CSV

            // Читаем данные из CSV файла
            readFromCsv(dirPath / schem_name / from_tables[k] / "1.csv", geted, from_tables[k], schem_name);

            // Получаем индекс текущего поля
            int index = tables.Get(from_tables[k]).find(rasp.Get(from_tables[k])[j]);

            List<List<string>> newFromed; // Новый список для хранения сформированных данных

            // Проходим по всем прочитанным строкам (кроме заголовка)
            for (int z = 0; z < geted.length - 1; z++) {
                List<string> formedRow; // Строка для хранения сформированных данных

                // Если сформированные данные уже существуют, копируем их
                if (formed.length > z) {
                    List<string> preFormed = formed[z];
                    copyList(preFormed, formedRow);
                }

                // Добавляем значение текущего поля к сформированной строке
                formedRow.push(geted[z + 1][index]);
                newFromed.push(formedRow); // Добавляем сформированную строку в новый список
            }

            // Очищаем предыдущие сформированные данные и обновляем их
            double_clear(formed);
            formed = newFromed;

            // Очищаем прочитанные данные
            double_clear(geted);
        }

        // Проверяем, какие данные сформировались и объединяем с предыдущими
        if (new_formed.length >= formed.length) {
            cross_joint(new_formed, formed); // Объединяем данные
            double_clear(formed); // Очищаем новые сформированные данные
        } else {
            cross_joint(formed, new_formed); // Объединяем данные
            double_clear(new_formed); // Очищаем предыдущие данные
            new_formed = formed; // Обновляем старые данные
        }
    }
}

// Функция для обработки условий WHERE 
void where_select(string& line, int& i, string& logOperator, List<string>& posl, List<List<string>>& new_formed, List<bool>& cmp) {
    // Пока есть логический оператор
    while (logOperator != "") {
        string first = take_string(line, i); // Получаем первое значение
        string op = take_string(line, i); // Получаем оператор
        if (op != "=") {
            throw runtime_error("Invalid operator"); // Проверяем на корректность оператора
        }
        string second = take_string(line, i); // Получаем второе значение

        // Проверяем, есть ли значения в списке полей
        if (posl.find(first) == -1) {
            first = unwrap_string(first); // Если нет, извлекаем строку
        }
        if (posl.find(second) == -1) {
            second = unwrap_string(second); // Если нет, извлекаем строку
        }

        // Находим индексы значений в списке полей
        int sod_first = posl.find(first);
        int sod_second = posl.find(second);

        List<bool> cmp_new; // Новый список для хранения результатов сравнения

        // Проходим по всем сформированным данным
        for (int j = 0; j < new_formed.length; j++) {
            string sr_first = first;
            string sr_second = second;

            // Если значение найдено в списке полей, берем его из сформированных данных
            if (sod_first != -1) {
                sr_first = new_formed[j][sod_first];
            }
            if (sod_second != -1) {
                sr_second = new_formed[j][sod_second];
            }
            // Сравниваем значения и добавляем результат в новый список
            cmp_new.push(sr_first == sr_second);
        }
        
        List<bool> cmp_res; // Список для хранения окончательных результатов

        // Обрабатываем логические операции
        for (int j = 0; j < cmp.length; j++) {
            if (logOperator == "AND") {
                cmp_res.push(cmp[j] && cmp_new[j]); // Если оператор AND, выполняем логическое И
            } else if (logOperator == "OR") {
                cmp_res.push(cmp[j] || cmp_new[j]); // Если оператор OR, выполняем логическое ИЛИ
            } else {
                throw runtime_error("Invalid logick"); // Проверяем на корректность логического оператора
            }
        }
        
        // Очищаем старые результаты и обновляем их
        cmp.clear();
        cmp_new.clear();
        cmp = cmp_res;

        logOperator = take_string(line, i); // Получаем следующий логический оператор
    }
}
// Функция для выполнения SQL-подобного запроса SELECT
void select(string& line, int& i, string& schem_name, HashTable<List<string>>& tables) {
    filesystem::path dirPath = "."; // Путь к директории
    List<string> selected_tables; // Список выбранных таблиц
    List<string> selected_columns; // Список выбранных колонок
    
    // Получаем имена таблиц и колонок из запроса
    get_names(line, i, selected_tables, selected_columns);
    
    string com = take_string(line, i); // Получаем следующее слово
    if (com != "FROM") { // Проверяем, что после SELECT идет FROM
        throw runtime_error("Wrong syntax"); // Если нет, выбрасываем исключение
    }

    List<string> from_tables; // Список таблиц, из которых берутся данные
    get_tables(line, i, from_tables); // Получаем список таблиц

    // Проверяем синтаксис выбранных таблиц
    if (!check_synt(selected_tables, from_tables)) {
        throw runtime_error("Wrong syntax"); // Если синтаксис неверен, выбрасываем исключение
    }

    HashTable<List<string>> rasp; // Хеш-таблица для хранения выбранных колонок для каждой таблицы

    // Заполняем хеш-таблицу
    for (int z = 0; z < from_tables.length; z++) {
        List<string> forRasp; // Список для хранения колонок текущей таблицы
        for (int j = 0; j < selected_tables.length; j++) {
            // Если выбранная таблица совпадает с текущей
            if (selected_tables[j] == from_tables[z]) {
                // Проверяем, что колонка еще не была добавлена
                if (forRasp.find(selected_columns[j]) != -1) {
                    throw runtime_error("Wrong syntax"); // Если колонка уже есть, выбрасываем исключение
                }
                forRasp.push(selected_columns[j]); // Добавляем колонку в список
            }
        }
        rasp.Add(from_tables[z], forRasp); // Добавляем список колонок в хеш-таблицу
    }

    List<List<string>> new_formed; // Список для хранения сформированных данных
    List<string> posl; // Список для хранения имен колонок

    // Формируем данные из выбранных таблиц
    form(schem_name, from_tables, rasp, posl, new_formed, tables);

    com = take_string(line, i); // Получаем следующее слово
    string logOperator = "AND"; // Логический оператор по умолчанию

    List<bool> cmp; // Список для хранения результатов условий
    for (int j = 0; j < new_formed.length; j++) {
        cmp.push(true); // Изначально все строки считаем подходящими
    }

    // Проверяем, есть ли условия WHERE
    if (com == "WHERE") {
        where_select(line, i, logOperator, posl, new_formed, cmp); // Обрабатываем условия
    }

    printFilteredRows(new_formed, cmp); // Печатаем отфильтрованные строки
    
    // Очищаем временные данные
    selected_tables.clear();
    selected_columns.clear();

    // Очищаем данные в хеш-таблице для отработанных таблиц
    for (int h = 0; h < from_tables.length; h++) {
        rasp.Get(from_tables[h]).clear();
    }

    from_tables.clear(); // Очищаем список таблиц
    posl.clear(); // Очищаем список колонок
    double_clear(new_formed); // Очищаем сформированные данные
}

// Функция для получения содержимого списка из строки
string take_list(string& cont, int& index) {
    string content; // Переменная для хранения содержимого
    bool get_content = 0; // Флаг для определения, нужно ли заканчивать сбор содержимого
    bool geting_content = 0; // Флаг для определения, находимся ли мы внутри скобок

    // Проходим по строке, пока не достигнем конца
    while (index < cont.length()) {
        if (get_content) {
            break; // Если флаг активен, выходим из цикла
        } else if (!get_content) {
            // Если нашли скобку
            if (cont[index] == '(' || cont[index] == ')') {
                if (geting_content) {
                    get_content = true; // Если уже внутри, устанавливаем флаг завершения
                } else {
                    geting_content = true; // Входим в режим сбора содержимого
                }
            } else if (geting_content) {
                content.push_back(cont[index]); // Собираем содержимое
            }
        }
        index++; // Переходим к следующему символу
    }
    return content; // Возвращаем собранное содержимое
}

// Функция для выполнения запроса INSERT
void insert(string& line, int& i, HashTable<List<string>>& tables, string& schem_name, int& limit) {
    filesystem::path dirPath = "."; // Путь к директории
    string table_name = take_string(line, i); // Получаем имя таблицы

    string cm = take_string(line, i); // Получаем следующее слово
    List<string> values; // Список для хранения значений

    // Проверяем, что после имени таблицы идет "VALUES"
    if (cm != "VALUES") {
        throw runtime_error("Wrong syntax"); // Если нет, выбрасываем исключение
    }

    string wraped = take_list(line, i); // Получаем содержимое в скобках
    bool more = true; // Флаг для продолжения обработки

    string value; // Переменная для хранения значения
    int j = 0; // Индекс для прохода по содержимому

    // Обрабатываем содержимое
    while (more) {
        if (j >= wraped.length()) {
            more = false; // Если достигли конца содержимого, выходим
        }
        if (wraped[j] == ',') { // Если нашли разделитель
            value = unwrap_string(value); // Извлекаем строку
            values.push(value); // Добавляем значение в список
            value = ""; // Сбрасываем переменную для следующего значения
            j++;
        } else {
            value.push_back(wraped[j]); // Собираем значение
        }
        j++; // Переходим к следующему символу
    }
    // Если осталась не добавленная строка, добавляем ее в список значений
    if (value != "") {
        values.push(unwrap_string(value));
    }

    // Проверяем количество переданных значений
    if (tables.Get(table_name).length > values.length) {
        throw runtime_error("Too few arguments"); // Если значений слишком мало, выбрасываем исключение
    }
    if (tables.Get(table_name).length < values.length) {
        throw runtime_error("Too many argumets"); // Если значений слишком много, выбрасываем исключение
    }

    // Проверяем значение первичного ключа
    int pk = readPrimaryKeyValue(dirPath / schem_name / table_name, table_name);
    if (pk + 1 > limit) {
        throw runtime_error("Out of limit"); // Если превышен лимит, выбрасываем исключение
    }

    // Добавляем значения в CSV файл
    appendToCsv(dirPath / schem_name / table_name / "1.csv", values, table_name, schem_name);
    setPrimaryKeyFile(dirPath / schem_name / table_name, table_name, pk + 1); // Обновляем значение первичного ключа

    values.clear(); // Очищаем список значений
}
// Функция для удаления строк из таблицы
void deleting(string& line, int& i, HashTable<List<string>>& tables, string& schem_name) {
    filesystem::path dirPath = "."; // Путь к директории

    List<List<string>> table; // Список для хранения данных таблицы
    List<List<string>> new_table; // Список для хранения данных таблицы, загруженных из CSV
    List<string> headers; // Заголовки столбцов таблицы
    int counter = 0; // Счетчик удаленных строк

    string table_name = take_string(line, i); // Получаем имя таблицы

    // Читаем данные из CSV файла
    readFromCsv(dirPath / schem_name / table_name / "1.csv", new_table, table_name, schem_name);

    // Переносим данные из новой таблицы в исходную таблицу, начиная со второго элемента
    for (int i = 1; i < new_table.length; i++) {
        List<string> tek = new_table[i]; // Получаем текущую строку
        List<string> nov; // Новый список для хранения строки
        copyList(tek, nov); // Копируем строку
        table.push(nov); // Добавляем строку в таблицу
    }

    // Копируем заголовки из первой строки
    List<string> newHeaders = new_table[0];

    // Формируем полные имена заголовков, включая имя таблицы
    for (int j = 0; j < newHeaders.length; j++) {
        headers.push(table_name + '.' + newHeaders[j]);
    }
    double_clear(new_table); // Очищаем новую таблицу

    List<bool> cmp; // Список для хранения условий удаления

    // Инициализируем список условий с истинными значениями
    for (int i = 0; i < table.length; i++) {
        cmp.push(true); // Изначально все строки считаем подходящими для удаления
    }

    string logOperator = "AND"; // Логический оператор по умолчанию

    // Проверяем наличие условий WHERE
    if (take_string(line, i) == "WHERE") {
        where_select(line, i, logOperator, headers, table, cmp); // Обрабатываем условия
    }

    List<List<string>> result; // Список для хранения результата
    List<string> origHead = tables.Get(table_name); // Получаем оригинальные заголовки таблицы
    List<string> headersDel; // Список заголовков для записи в CSV

    copyList(origHead, headersDel); // Копируем заголовки

    result.push(headersDel); // Добавляем заголовки в результат

    // Проходим по таблице и отбираем строки, которые не соответствуют условиям
    for (int i = 0; i < table.length; i++) {
        if (!cmp[i]) {
            result.push(table[i]); // Добавляем строку в результат, если она подходит
        } else {
            counter++; // Увеличиваем счетчик удаленных строк
        }
    }

    // Записываем обновленные данные обратно в CSV
    writeToCsv(dirPath / schem_name / table_name / "1.csv", result, table_name, schem_name);
    
    // Обновляем значение первичного ключа
    int pk = readPrimaryKeyValue(dirPath / schem_name / table_name, table_name);
    pk -= counter; // Уменьшаем значение первичного ключа на количество удаленных строк
    setPrimaryKeyFile(dirPath / schem_name / table_name, table_name, pk); // Сохраняем новое значение первичного ключа

    double_clear(table); // Очищаем таблицу
    headers.clear(); // Очищаем заголовки
}
