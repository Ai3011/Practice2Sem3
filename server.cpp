#include <iostream>  
#include <thread>  
#include <mutex>  
#include <memory>  
#include <netinet/in.h> // Для работы с сокетами  
#include <unistd.h> // Для close()  
#include <cstring> // Для memset  
#include <stdexcept>  
#include "Commands.h" // Подключение вашей реализации команд  
  
using namespace std;  
  
std::mutex db_mutex; // Мьютекс для синхронизации доступа к БД  
  
// Функция для обработки команд 
void console_parse(string& schem_name, HashTable<List<string>>& tables, List<string>& tables_names, int& limit, const string& command, string& response) { 
    try { 
        string command_copy = command; // Создаём копию строки команды для модификации 
        string command_word; 
        int i = 0; 
 
        // Извлекаем команду из строки 
        while (i < command_copy.length() && command_copy[i] != ' ') { 
            command_word.push_back(command_copy[i]); 
            i++; 
        } 
        i++; // Пропускаем пробел 
 
        // Обработка команд 
        if (command_word == "SELECT") { 
            select(command_copy, i, schem_name, tables); // Используем копию 
            response = "SELECT command executed"; 
        } else if (command_word == "INSERT") { 
            string cm = take_string(command_copy, i); // Используем копию 
            if (cm != "INTO") { 
                throw runtime_error("Wrong syntax"); 
            } 
            insert(command_copy, i, tables, schem_name, limit); // Используем копию 
            response = "INSERT command executed"; 
        } else if (command_word == "DELETE") { 
            string cm = take_string(command_copy, i); // Используем копию 
            if (cm != "FROM") { 
                throw runtime_error("Wrong syntax"); 
            } 
            deleting(command_copy, i, tables, schem_name); // Используем копию 
            response = "DELETE command executed"; 
        } else { 
            throw runtime_error("Unknown command"); 
        } 
    } catch (const runtime_error& err) { 
        response = "Error: " + string(err.what()); 
    } 
}
 
void handle_client(int client_socket, string& schem_name,     
                   shared_ptr<HashTable<List<string>>> tables,     
                   shared_ptr<List<string>> tables_names, int limit) {    
    char buffer[1024] = {0};    
    bool running = true;  // состояние для продолжения работы 
   
    while (running) {  // Ожидаем следующую команду 
        // Получаем данные от клиента    
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);    
        if (bytes_received <= 0) {    
            cerr << "Error receiving data or client disconnected" << endl;    
            break;  // Завершаем цикл, если ошибка получения или клиент отключился   
        }    
        buffer[bytes_received] = '\0'; // Завершаем строку    
        string command(buffer);    
        string response;  
   
        try {   
            std::lock_guard<std::mutex> lock(db_mutex); // Защищаем работу с БД   
            console_parse(schem_name, *tables, *tables_names, limit, command, response); // Обрабатываем команду   
             
            if (command == "EXIT") {  // Проверка на команду выхода 
                response = "Exiting..."; 
                send(client_socket, response.c_str(), response.size(), 0); 
                running = false;  // Завершаем цикл, если команда EXIT   
            } else { 
                send(client_socket, response.c_str(), response.size(), 0); // Отправляем результат клиенту 
            } 
             
        } catch (const std::exception& e) {   
            response = "Error: " + string(e.what());   
            send(client_socket, response.c_str(), response.size(), 0);   
        }  
    }  
   
    close(client_socket);  // Закрываем соединение после выхода из цикла 
}
  
int main() {   
    auto tables = make_shared<HashTable<List<string>>>();   
    auto tables_names = make_shared<List<string>>();   
    string schem_name;   
    int limit;   
  
    // Загрузка данных из JSON-файла   
    try {   
        getJSON(*tables, *tables_names, schem_name, limit);   
    } catch (const std::exception& e) {   
        cerr << "Failed to load JSON data: " << e.what() << endl;   
        return EXIT_FAILURE; 
    }   
  
    // Настройка сервера  
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);   
    if (server_socket < 0) throw runtime_error("Unable to create socket");   
  
    sockaddr_in server_addr{};   
    server_addr.sin_family = AF_INET;   
    server_addr.sin_addr.s_addr = INADDR_ANY;   
    server_addr.sin_port = htons(7432);   
  
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {   
        close(server_socket);   
        throw runtime_error("Bind failed");   
    }   
    if (listen(server_socket, 5) < 0) {   
        close(server_socket);   
        throw runtime_error("Listen failed");
}   
  
    cout << "Server is listening on port 7432..." << endl;   
  
    while (true) {   
        int client_socket = accept(server_socket, nullptr, nullptr);   
        if (client_socket < 0) {   
            cerr << "Accept failed" << endl;   
            continue;  
        }   
          
        thread t(handle_client, client_socket, ref(schem_name), tables, tables_names, limit);   
        t.detach();  
    }   
  
    close(server_socket);   
    return 0;   
} 
