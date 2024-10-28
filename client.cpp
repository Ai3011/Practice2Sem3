#include <iostream>        
#include <arpa/inet.h>    
#include <unistd.h>    
 
using namespace std;    
 
int main() {    
    int sock = 0;    //сокет
    struct sockaddr_in serv_addr;    //структура хранящая данные для соединения
 
    // Создание сокета 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {    // протокол + потоковый сокет
        cout << "Socket creation error" << endl;    
        return -1;    
    }    
 
    serv_addr.sin_family = AF_INET;    // протокол IPv-4
    serv_addr.sin_port = htons(7432);    //устанавливаем порт 7432 и преобразуем в сетевой порядок байтов
 
    // Преобразование адреса 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {    //преобразуем адрес в числовой формат
        cout << "Invalid address/ Address not supported" << endl;    
        return -1;    
    }    
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {    //подключение к серверу
        cout << "Connection failed" << endl;    
        return -1;    
    }    
 
    while (true) {    
        string command;    // строка для команд
        cout << "Enter your command (e.g., SELECT, INSERT, EXIT to quit): ";    
        getline(cin, command);    
 
        // Проверка на пустую команду 
        if (command.empty()) { 
            cout << "Error: Empty command. Please enter a valid command." << endl; 
            continue; 
        } 
 
        // Проверка на команду завершения    
        if (command == "EXIT") {    
            cout << "Exiting client..." << endl;    
            break;    
        }    
 
        // Отправка команды серверу    
        send(sock, command.c_str(), command.length(), 0);    // преобразуя в строку char ов
 
        // Очистка буфера перед чтением  
        char buffer[1024] = {0};    // ответ сервера
          
        // Получение ответа от сервера    
        int bytes_received = read(sock, buffer, sizeof(buffer));    // проверяем сколько получили байт от сервера
        if (bytes_received > 0) {    // елси получили хоть что-то то выводим,иначе сервер не дал нам ответа
            cout << "Response from server: " << buffer << endl;    
        } else {    
            cout << "Failed to receive response" << endl;    
        }    
    }    
 
    close(sock);    
    return 0;    
}
