#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
using namespace std;

#define PORT 9001
#define BUFFER_SIZE 1024
#define ACK "ACK"
#define QUEUE_NAME "/test_queue"
#define MAX_SIZE 1024

mqd_t mq;

void producer(int socketProducer) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        int read_bytes = read(socketProducer, buffer, BUFFER_SIZE);
        if (read_bytes <= 0) {
            cout << "Producer disconnected or error occurred." << endl;
            break;
        }

        // Send the message to the queue
        if (mq_send(mq, buffer, read_bytes, 0) == -1) {
            perror("mq_send");
            break;
        }

        send(socketProducer, ACK, strlen(ACK), 0);
    }

    close(socketProducer);
}

void consumer(int socketConsumer) {
    char buffer[BUFFER_SIZE] = {0};
    char ackBuf[3] = {0};

    while (true) {
        ssize_t bytes_read = mq_receive(mq, buffer, BUFFER_SIZE, NULL);
        if (bytes_read <= 0) {
            perror("mq_receive");
            break;
        }

        int sent_bytes = send(socketConsumer, buffer, bytes_read, 0);
        cout << sent_bytes <<endl;
        if (sent_bytes <= 0) {
            cout << "Consumer disconnected or error occurred." << endl;
            break;
        }

        read(socketConsumer, ackBuf, 3);
    }

    close(socketConsumer);
}

void setConsumerOrProducer(int socket, const string& type) {
    pid_t x = fork();
    if(x == 0){
        if (type == "PRODUCER") {
            mq_unlink(QUEUE_NAME);
            struct mq_attr attr;
            attr.mq_flags = 0;
            attr.mq_maxmsg = 10;
            attr.mq_msgsize = BUFFER_SIZE;
            attr.mq_curmsgs = 0;

            mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);
            if (mq == -1) {
                perror("mq_open");
                exit(1);
            }

            producer(socket);
        } else if (type == "CONSUMER") {
            mq = mq_open(QUEUE_NAME, O_RDONLY);
            if (mq == -1) {
                perror("mq_open");
                exit(1);
            }

            consumer(socket);
        }
    }
}

int main(int argc, char *argv[]) {
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
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        cout << "waiting for connection" << endl;
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        char buffer[10];
        int read_bytes = read(new_socket, buffer, 10);
        if (read_bytes <= 0) {
            close(new_socket);
            continue;
        }

        string type(buffer);
        setConsumerOrProducer(new_socket, type);
    }

    return 0;
}
