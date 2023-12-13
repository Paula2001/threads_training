#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <time.h>
#include <semaphore.h>
#define PORT 8081
#define BUFFER_SIZE 1024
#define QUERY_PARAM_FILE_KEY "file_name"
#define CHUNK_SIZE 100  // bytes
#define DELAY_TIME 5    // second

sem_t block;

char* readFileContent(const char *filename) {
    FILE *file;
    char *content;
    long file_size;

    file = fopen(filename, "r"); // Open the file for reading
    if (file == NULL) {
        perror("Failed to open the file");
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Allocate memory for the entire content
    content = (char *)malloc(file_size + 1);
    if (content == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the file into memory and null-terminate the string
    fread(content, 1, file_size, file);
    content[file_size] = '\0';

    fclose(file); // Close the file
    return content;
}

char* response(char* fileName) {
    char* fileContent = readFileContent(fileName);

    if(fileContent == NULL){
        const char* notfound = "HTTP/1.1 404 Not Found\n\n"
               "404 Not Found\n";
        char *response = static_cast<char *>(malloc(strlen(notfound) + 1));

        strcpy(response,notfound);
        return response;
    }

    const char *OKheader =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n\r\n";
    char *response = static_cast<char *>(malloc(strlen(OKheader) + strlen(fileContent) + 1));

    // Construct the response
    strcpy(response, OKheader);
    strcat(response, fileContent);
    return response;
}

char* get_query_param_value(const char* query, const char* key) {
    if (query == NULL || key == NULL) {
        return NULL;
    }

    // Copy the query string to a mutable buffer
    char* buffer = strdup(query);
    char* value = NULL;

    // Tokenize the query string on '&'
    char* token = strtok(buffer, "&");
    while (token != NULL) {
        // Check if the token contains the key
        if (strncmp(token, key, strlen(key)) == 0 && token[strlen(key)] == '=') {
            // Find the start of the value
            char* value_start = strchr(token, '=') + 1;
            value = strdup(value_start);
            break;
        }
        token = strtok(NULL, "&");
    }

    // Clean up and return the result
    free(buffer);
    return value;
}

void *handle_connection(void *socket_desc) {
    int* s = (int *)socket_desc;
    int sock = (uintptr_t)s;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    // Read the request
    read_size = read(sock, buffer, BUFFER_SIZE - 1);
    sem_wait(&block);
    if (read_size > 0) {
        printf("Received request:\n%s\n", buffer);

        // Check if the request is a GET request
        if (strncmp(buffer, "GET", 3) == 0) {
            char *start = strchr(buffer, ' ') + 1;
            char *end = strchr(start, ' ');

            // Extract the path and query
            char path_and_query[1024] = {0};
            strncpy(path_and_query, start, end - start);

            // Optionally, separate the path and the query
            char *query = strchr(path_and_query, '?');
            if (query) {
                // Skip over the '?' to get the query parameters
                query++;
                printf("Query params: %s\n", query);
            }
            struct timespec delay;
            delay.tv_sec = DELAY_TIME;
            delay.tv_nsec = 0;
            nanosleep(&delay, NULL); // sleep for 1 second
            char* r = response(get_query_param_value(query, QUERY_PARAM_FILE_KEY));
            // Send the response
            write(sock, r, strlen(r));
        }
    }
    sem_post(&block);
    close(sock);
    pthread_exit(NULL);
}

int main() {
    sem_init(&block, 0, 1);
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