#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT 9132

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        return -1;
    }

    while (true) {
        std::string command;
        std::cout << "Enter command (UP, DOWN, LEFT, RIGHT, SHOW, EXIT): ";
        std::cin >> command;

        // Send command to server
        send(sock, command.c_str(), command.length(), 0);

        if (command == "EXIT") {
            break;
        }

        // Wait for response from server
        int valread = read(sock, buffer, 1024);
        std::cout << "Server: \n" << buffer << std::endl;

        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    }

    close(sock);
    return 0;
}
