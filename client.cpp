#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;
#define PORT 123
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
         cerr << "Invalid address/ Address not supported\n";
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         cerr << "Connection failed\n";
        return -1;
    }

     cout << "Connected to server\n";

    while (true) {
         string message;
         cout << "Enter your message: ";
         getline( cin, message);

        send(sock, message.c_str(), message.size(), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);

         cout << "Server replied: " << buffer << "\n";

        if (message == "Quit" &&  string(buffer) == "Goodbye") {
             cout << "Exiting...\n";
            break;
        }
    }

    close(sock);
    return 0;
}
