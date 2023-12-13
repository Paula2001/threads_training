#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>

#define PORT 8081
#define BUFFER_SIZE 1024

void *handle_connection(void *socket_desc) {
    int* s = (int *)socket_desc;
    int sock = (uintptr_t)s;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    // Read the request
    read_size = read(sock, buffer, BUFFER_SIZE - 1);
    if (read_size > 0) {
        printf("Received request:\n%s\n", buffer);

        // Check if the request is a GET request
        if (strncmp(buffer, "GET", 3) == 0) {
            char response[] =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<html><body><h1>Hello, World!</h1></body></html>\r\n";

            // Send the response
            write(sock, response, strlen(response));
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addr_len = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", PORT);

    // Accept incoming connections
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
        if (new_socket < 0) {
            perror("Accept");
            continue;
        }

        pthread_t thread_id;

        if (pthread_create(&thread_id, NULL, handle_connection, (void*)new_socket) < 0) {
            perror("Could not create thread");
            continue;
        }

        // Optionally detach the thread
        pthread_detach(thread_id);
    }

    return 0;
}