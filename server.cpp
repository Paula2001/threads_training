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

char buf;
sem_t producerSem, consumerSem;
std::queue<string> q;
mutex mtx; // Mutex for thread synchronization
bool isConsumerConnected = false;

void setConsumerAndProducer(string clientType, int socket) {
    const char* ACK = "ACK";
    sem_wait(&consumerSem); // Start
//    cout << "This is client type = " << clientType <<endl;
    
    if (clientType == "PRODUCER") {
        cout << "Connected with a PRODUCER" << endl;
        send(socket, ACK, strlen(ACK), 0);
    } else if (clientType == "CONSUMER") {
        cout << "Connected with a CONSUMER" << endl;
        isConsumerConnected = true;
        send(socket, ACK, strlen(ACK), 0);
    }

    sem_post(&consumerSem); // End
}

void Producer(int socket) {
    if(q.size() < 10) {
        char buffer[1024] = {0};
        int i = 0;
        string test = "ACK";
        while (true) {
            int read_bytes = read(socket, buffer, 1024);
            if (read_bytes > 0) {
                cout << "Producer: " << buffer << endl;
                mtx.lock();
                q.emplace(buffer);
                mtx.unlock();
                send(socket, test.c_str(), test.length(), 0);
                if(q.size() > 10) {
                    break;
                }
            }else{
                break;
            }
        }
    }
}

void Consumer(int socket) {
    if(q.size() > 10 && isConsumerConnected){
        char buffer[1024] = {0};
        string test = "ACK";

        while (!q.empty()) {
            send(socket, q.front().c_str(), q.front().length(), 0);
            q.pop(); // Remove the front element
        }

        send(socket, test.c_str(), test.length(), 0);
    }
}

void handleConnection(int new_socket) {
    char buffer[1024] = {0};
    int read_bytes = read(new_socket, buffer, 1024);
    string clientType(buffer);
    setConsumerAndProducer(clientType, new_socket);
    Producer(new_socket);
    Consumer(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    sem_init(&producerSem, 0, 0);
    sem_init(&consumerSem, 0, 1);

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
        cout << "Waiting for connections..." << endl;

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        std::thread clientThread(handleConnection, new_socket);
        clientThread.detach(); // Detach the thread
    }

    sem_destroy(&producerSem);
    sem_destroy(&consumerSem);

    return 0;
}
