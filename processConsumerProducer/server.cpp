#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <mutex>
#include <queue>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

#define PORT 9001
#define N 1000
#define BUFFER_SIZE 1024
#define PIPE_BUFFER_SIZE 1024
#define ACK "ACK"
#define SEM_EMPTY "/semaphore_empty"
#define SEM_FULL "/semaphore_full"
#define FIFO_PATH "mypipe"

std::queue<string> q;

sem_t *empt ;
sem_t *full;

void producer(int socketProducer) {
    auto pipeWriteOnly = open( FIFO_PATH, O_WRONLY);
    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read_bytes = read(socketProducer, buffer, BUFFER_SIZE);
        if (read_bytes <= 0) {
            cout << "Producer disconnected or error occurred." << endl;
            break; // Exiting the loop and ending the thread
        }
        auto sizeOfChar = to_string(strlen(buffer)).c_str();
        write(pipeWriteOnly,sizeOfChar , 2);
        write(pipeWriteOnly, buffer, read_bytes);

        sem_wait(empt);
        cout << "this is a test" << endl;

        cout << "Producer: " << buffer << endl;
        q.emplace(buffer);
        send(socketProducer, ACK, strlen(ACK), 0);

        sem_post(full);
    }

    close(socketProducer);
}

void consumer(int socketConsumer) {
    char ackBuf[3] = {0};
    char lengthBuffer[1] = {0};
    char wordBuffer[50] = {0};
    auto pipeReadOnly = open( FIFO_PATH, O_RDONLY );
    while (true) {
        sem_wait(full);
        read(pipeReadOnly, lengthBuffer, 2);
        read(pipeReadOnly, wordBuffer, atoi(lengthBuffer));

        int sent_bytes = send(socketConsumer, wordBuffer, atoi(lengthBuffer), 0);
        cout << "===" <<endl;
        cout << lengthBuffer <<endl;
        cout << "---" <<endl;
        cout << wordBuffer <<endl;
        cout << "===" <<endl;
        if (sent_bytes <= 0) {
            cout << "Consumer disconnected or error occurred." << endl;
            break; // Exiting the loop and ending the thread
        }
        sem_post(empt);

        read(socketConsumer, ackBuf, 3);

    }

    close(socketConsumer);
}

void setConsumerOrProducer(int socket, const string& type) {
    pthread_t thread;
    pid_t process = fork();
    if(process == 0){ // INF: this is a child process
        if (type == "PRODUCER") {
            producer(socket);
        } else if (type == "CONSUMER") {
            consumer(socket);
        }
    }

    pthread_detach(thread); // Detach the thread
}

int main(int argc, char *argv[]) {
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_EMPTY);
    empt = sem_open(SEM_EMPTY, O_CREAT, 0644, N);
    full = sem_open(SEM_FULL, O_CREAT, 0644, 0);
//    if (pipe(pipefd) == -1) {
//        perror("pipe");
//        return 1;
//    }




    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        cout << "waiting for connection" <<endl;
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        char buffer[10];
        int read_bytes = read(new_socket, buffer, 10);
        if (read_bytes <= 0) {
            close(new_socket);
            continue; // Ignore this connection if an error occurred
        }

        string type(buffer);
        setConsumerOrProducer(new_socket, type);
    }

    return 0;
}
