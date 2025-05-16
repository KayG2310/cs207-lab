#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

using namespace std;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 2)) == 0) {
        cerr << "Socket creation failed\n";
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed\n";
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed\n";
        return -1;
    }

    cout << "Server started on port " << PORT << ". Waiting for connections...\n";

    int addrlen = sizeof(address);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        cerr << "Accept failed\n";
        return -1;
    }

    cout << "Client connected\n";

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(new_socket, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            cout << "Connection closed by client\n";
            break;
        }

        cout << "Received from client: " << buffer << "\n";

        string reply;
        if (string(buffer) == "Quit") {
            reply = "Goodbye";
            send(new_socket, reply.c_str(), reply.size(), 0);
            break;
        } else {
            reply = "OK";
        }

        send(new_socket, reply.c_str(), reply.size(), 0);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
