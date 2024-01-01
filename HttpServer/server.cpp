#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#define PORT 8081
#define BUFFER_SIZE 1024
#define QUERY_PARAM_FILE_KEY "file_name"
#define CHUNK_SIZE 100  // bytes
#define DELAY_TIME 1    // second

sem_t block;
int pipefd[2];
pid_t pid;
char buffer[BUFFER_SIZE];

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

char* response() {

    // Read the output from the child process
    ssize_t numBytes = read(pipefd[0], buffer, sizeof(buffer) - 1);
    if (numBytes < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[numBytes] = '\0'; // Null-terminate the string

    // Construct the response
    const char *OKheader =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n\r\n";
    char *response = (char *)malloc(strlen(OKheader) + strlen(buffer) + 1);
    strcpy(response, OKheader);
    strcat(response, buffer);

    close(pipefd[0]); // Close read end
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
            char *spacePtr = strchr(value, ' '); // Find the first space
            int index = spacePtr - value; // Calculate the index
            value[index] = '\0';

            break;
        }
        token = strtok(NULL, "&");
    }

    // Clean up and return the result
    free(buffer);
    return value;
}

void *handle_connection(int sock) {
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    // Read the request
    pid_t pid = fork();
    if(pid == 0) {
        // Child process
        sem_wait(&block);
        read_size = read(sock, buffer, BUFFER_SIZE - 1);
        if (read_size > 0) {
            buffer[read_size] = '\0'; // Null-terminate the request string
            printf("Received request:\n%s\n", buffer);

            // Check if the request is a GET request
            if (strncmp(buffer, "GET", 3) == 0) {
                // Extract the query parameter
                char *query = strstr(buffer, "?");
                printf("%s\n", query);
                char *fileName = NULL;
                if (query) {
                    query++; // Move past the '?'
                    fileName = get_query_param_value(query, QUERY_PARAM_FILE_KEY);
                    printf("%s\n", fileName);
                }
                // Prepare for sending the command output
                const char *OKheader =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n\r\n";
                write(sock, OKheader, strlen(OKheader));

                // Redirect STDOUT to socket
                dup2(sock, STDOUT_FILENO);

                    // Execute the command with the specified file name
                execlp(fileName, NULL);


                // If execlp returns, it must have failed
                perror("execlp");
                exit(EXIT_FAILURE);
            }
        }
        sem_post(&block);
        close(sock);
        exit(0);
    } else {
        // Parent process
        close(sock);
    }
    return 0;
}



int main() {
    sem_init(&block, 1, 1);
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

        handle_connection(new_socket);


        // Optionally detach the thread
    }

    return 0;
}