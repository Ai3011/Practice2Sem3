#include <iostream> 
#include <cstring> 
#include <arpa/inet.h> 
#include <unistd.h> 
 
using namespace std; 
 
int main() { 
    int sock = 0; 
    struct sockaddr_in serv_addr; 
 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout << "Socket creation error" << endl; 
        return -1; 
    } 
 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(7432); 
 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { 
        cout << "Invalid address/ Address not supported" << endl; 
        return -1; 
    } 
 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        cout << "Connection failed" << endl; 
        return -1; 
    } 
 
    string command; 
    cout << "Enter your command (e.g., SELECT, INSERT): "; 
    getline(cin, command); 
 
    send(sock, command.c_str(), command.length(), 0); 
 
    char buffer[1024] = {0}; 
    int bytes_received = read(sock, buffer, sizeof(buffer)); 
    if (bytes_received > 0) { 
        cout << "Response from server: " << buffer << endl; 
    } else { 
        cout << "Failed to receive response" << endl; 
    } 
 
    close(sock); 
    return 0; 
}
