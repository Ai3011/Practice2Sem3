#include <iostream> 
#include <thread> 
#include <mutex> 
#include <memory> 
#include <netinet/in.h> // Для работы с сокетами 
#include <unistd.h>     // Для close() 
#include <cstring>     
#include <stdexcept>  // исключения
#include "Commands.h"    
using namespace std; 
 
mutex db_mutex; // Мьютекс для синхронизации доступа к БД 
 
// Функция для обработки команд 
void console_parse(string& schem_name, HashTable<List<string>>& tables, List<string>& tables_names, int& limit, const string& command, string& response) { 
    try { 
        string command_copy = command; // Создаём копию строки команды для модификации 
        string command_word; //  считываем первое слово из строки с командой.
        int i = 0; // для прохода по строке
 
        // Извлекаем команду из строки 
        while (i < command_copy.length() && command_copy[i] != ' ') { 
            command_word.push_back(command_copy[i]); 
            i++; 
        } 
        i++; // Пропускаем пробел 
 
        // Обработка команд 
        if (command_word == "SELECT") { 
            response = select(command_copy, i, schem_name, tables); // записываем результат команды SELECT в эту строчку (строка для пользователя)
        } else if (command_word == "INSERT") { 
            string cm = take_string(command_copy, i); // Проверяем второе слово после insert
            if (cm != "INTO") { 
                throw runtime_error("Wrong syntax"); 
            } 
            insert(command_copy, i, tables, schem_name, limit); // Используем копию 
            response = "INSERT command executed";  // строка для пользователя
        } else if (command_word == "DELETE") { 
            string cm = take_string(command_copy, i); // проверяем слово после DELETE
            if (cm != "FROM") { 
                throw runtime_error("Wrong syntax"); 
            } 
            deleting(command_copy, i, tables, schem_name); // выполняем команду
            response = "DELETE command executed"; //ответ пользователю
        } else { 
            throw runtime_error("Unknown command"); 
        } 
 
        this_thread::sleep_for(chrono::seconds(5)); // Задержка для проверки мьютекса
 
    } catch (const runtime_error& err) { 
        response = "Error: " + string(err.what()); // ответ пользователю в случае ошибки
    } 
} 
 
// Функция для обработки запросов клиента 
void handle_client(int client_socket, string& schem_name, shared_ptr<HashTable<List<string>>> tables, shared_ptr<List<string>> tables_names, int limit) { 
    char buffer[1024] = {0}; //  команда клиента
    bool running = true;  // состояние для продолжения работы 
 
    // Логирование подключения клиента 
    cout << "Client connected (Socket: " << client_socket << ")" << endl; 
 
    while (running) {  // Ожидаем следующую команду 
        // Получаем данные от клиента 
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0); // если ничего не получили от клиента ,то ошибка
        if (bytes_received <= 0) { 
            cerr << "Error receiving data or client disconnected (Socket: " << client_socket << ")" << endl; 
            break;  // Завершаем цикл, если ошибка получения или клиент отключился 
        } 
        buffer[bytes_received] = '\0'; // Завершаем строку 
        string command(buffer);  // преобразуем  команду клиента в строку под названием command для работы далее

        if (command.empty()) { // проверка на пустой ввод клиента
            string response = "Error: Empty command received.";
            send(client_socket, response.c_str(), response.size(), 0);
            continue; // Пропускаем текущую итерацию, если команда пустая
        }

        string response; // строка для ответа клиенту
 
        // Логируем полученную команду 
        cout << "Received command from client (Socket: " << client_socket << "): " << command << endl; 
 
        // Проверка состояния мьютекса 
        if (!db_mutex.try_lock()) { 
            response = "Currently, another user is working with the database."; // ответ пользователю в случае если с БД работает другой пользователь
            send(client_socket, response.c_str(), response.size(), 0); 
            continue; // Пропускаем текущую итерацию, если мьютекс заблокирован 
        } 
 
        try { 
            console_parse(schem_name, *tables, *tables_names, limit, command, response); // Обрабатываем команду 
 
            // Логируем результат выполнения команды 
            if (response.find("Error:") == string::npos) { 
                cout << "Command executed successfully (Socket: " << client_socket << "): " << command << endl; 
            } else { 
                cout << "Command execution failed (Socket: " << client_socket << "): " << response << endl; 
            } 
 
            if (command == "EXIT") {  // Проверка на команду выхода 
                response = "Exiting..."; 
                send(client_socket, response.c_str(), response.size(), 0); 
                running = false;  // Завершаем цикл, если команда EXIT 
            } else { 
                send(client_socket, response.c_str(), response.size(), 0); // Отправляем результат клиенту 
            } 
        } catch (const exception& e) { 
            response = "Error: " + string(e.what()); 
            send(client_socket, response.c_str(), response.size(), 0); 
            cout << "Command execution failed (Socket: " << client_socket << "): " << response << endl; 
        } 
 
        db_mutex.unlock(); // Освобождаем мьютекс после обработки команды 
    } 
 
    // Логирование отключения клиента 
    cout << "Client disconnected (Socket: " << client_socket << ")" << endl; 
 
    close(client_socket);  // Закрываем соединение после выхода из цикла 
} 
 
int main() { 
    auto tables = make_shared<HashTable<List<string>>>();  //указатель на хэш-таблицу
    auto tables_names = make_shared<List<string>>();  // указатель на лист
    string schem_name; // схема бд
    int limit; //лимит значений в бд
 
    // Загрузка данных из JSON-файла 
    try { 
        getJSON(*tables, *tables_names, schem_name, limit); //считываем файл
    } catch (const exception& e) { 
        cerr << "Failed to load JSON data: " << e.what() << endl; 
        return EXIT_FAILURE; 
    } 
 
    // Настройка сервера 
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);  //создание сокета сервера
    if (server_socket < 0) {
     throw runtime_error("Unable to create socket"); 
    }
 
    sockaddr_in server_addr{};  //хранение  Ip-адреса и порта
    server_addr.sin_family = AF_INET;  // протокол IPv-4
    server_addr.sin_addr.s_addr = INADDR_ANY; // любой IP адрес
    server_addr.sin_port = htons(7432);  // порт 7432
 
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {  //привязывание сокета с портом и адресом
        close(server_socket); 
        throw runtime_error("Bind failed"); 
    } 
    if (listen(server_socket, 5) < 0) {  // переводим сервер в режим ожидания и до 5 подключений.
        close(server_socket); 
        throw runtime_error("Listen failed"); 
    } 
 
    cout << "Server is listening on port 7432..." << endl; 
 
    while (true) { 
        int client_socket = accept(server_socket, nullptr, nullptr); //принимаем соединение и создаем сокет клиента
        if (client_socket < 0) { 
            cerr << "Accept failed" << endl; // подключение не удалось
            continue; 
        } 
 
        cout << "New client accepted (Socket: " << client_socket << ")" << endl; 
 
        thread t(handle_client, client_socket, ref(schem_name), tables, tables_names, limit); //создаем поток (клиента)
        t.detach();  // Запускаем поток и продолжаем принимать новых клиентов 
    } 
 
    close(server_socket); 
    return 0; 
} 
