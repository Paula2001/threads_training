#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <queue>
#include <semaphore.h>

using namespace std;

#define PORT 9000

sem_t producerSem;
sem_t consumerSem;
sem_t setConsumerAndProducerSem;
std::queue<string> q;
mutex mtx; // Mutex for thread synchronization
bool isConsumerConnected = false;
bool isProducerConnected = false;
int socketProducer = 0;
int socketConsumer = 0;
void setConsumerAndProducer(char *clientType, int socket) {
    const char* ACK = "ACK";
    mtx.lock();

    char first8Bytes[8];
    memcpy(first8Bytes, clientType, 8);
    string c(first8Bytes);
    if (c == "PRODUCER") {
        cout << "Connected with a PRODUCER" << endl;
        socketProducer = socket;
        isProducerConnected = true;
        send(socket, ACK, strlen(ACK), 0);
    } else if (c == "CONSUMER") {
        cout << "Connected with a CONSUMER" << endl;
        isConsumerConnected = true;
        socketConsumer = socket;
        send(socket, ACK, strlen(ACK), 0);
    }
    mtx.unlock();
}

void Producer(int socket) {
    int queueSize = q.size();
    if(queueSize < 10 && isProducerConnected) {
        char buffer[1024] = {0};
        int i = 0;
        string test = "ACK";
        sem_wait(&consumerSem);
        while (true) {
            int read_bytes = read(socket, buffer, 1024);
            if (read_bytes > 0) {
                cout << "Producer: " << buffer << endl;
                q.emplace(buffer);
                send(socket, test.c_str(), test.length(), 0);
                if(q.size() > 10) {
                    break;
                }
            }else{
                break;
            }
        }
        sem_post(&consumerSem); // End
        sleep(3);
    }
}

void Consumer(int socket) {
    sem_wait(&producerSem);
    if(q.size() > 10 && isConsumerConnected){
        char buffer[1024] = {0};
        string test = "ACK";

        while (!q.empty()) {
            send(socket, q.front().c_str(), q.front().length(), 0);
            q.pop(); // Remove the front element
        }

        send(socket, test.c_str(), test.length(), 0);
    }
    sem_post(&producerSem); // End
}

void handleConnection(int new_socket) {
    char buffer[1024] = {0};
    int read_bytes = read(new_socket, buffer, 1024);
    setConsumerAndProducer(buffer, new_socket);
    Producer(socketProducer);
    Consumer(socketConsumer);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    sem_init(&producerSem, 0, 1);
    sem_init(&consumerSem, 0, 1);
    sem_init(&setConsumerAndProducerSem, 0, 1);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
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

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        if(!isProducerConnected || !isConsumerConnected){
            cout << "Waiting for connections..." << endl;

            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }
        }

        std::thread clientThread(handleConnection, new_socket);
        clientThread.join();
        //clientThread.detach(); // Detach the thread
    }

    sem_destroy(&producerSem);
    sem_destroy(&consumerSem);

    return 0;
}
