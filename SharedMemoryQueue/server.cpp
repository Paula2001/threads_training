#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

using namespace std;

#define PORT 9000
#define BUFFER_SIZE 1024
#define ACK "ACK"
#define MSG_NUM 1000
#define MSG_LEN 1024

struct ShmQueue {
    sem_t m_full, m_empty, m_mutex;
    char m_data[MSG_NUM][MSG_LEN];
    int m_first_msg, m_last_msg;
    bool m_consumer_connected;
};

struct ShmQueueZ {
    sem_t m_full, m_empty, m_mutex;
    char m_data[MSG_NUM][MSG_LEN];
    int m_first_msg, m_last_msg;
    bool m_consumer_connected;
};

ShmQueue* shmQueue;
ShmQueueZ* shmQueueZ;

int isFirstLetterBetweenAandL(const char *str) {
    if (str[0] == '\0') {  // Check if the string is empty
        return 0; // Return 0 (false) if the string is empty
    }

    char firstLetter = tolower(str[0]);  // Convert the first letter to lowercase
    return (firstLetter >= 'a' && firstLetter <= 'l'); // Check if it's between 'a' and 'l'
}

void down(sem_t *sem) {
    sem_wait(sem);
}

void up(sem_t *sem) {
    sem_post(sem);
}

int my_send(ShmQueue *t_myq, char *t_msg, int t_len) {
    down(&t_myq->m_empty);
    down(&t_myq->m_mutex);

    // Insert item
    strncpy(t_myq->m_data[t_myq->m_last_msg], t_msg, MSG_LEN);
    t_myq->m_last_msg = (t_myq->m_last_msg + 1) % MSG_NUM;

    up(&t_myq->m_mutex);
    up(&t_myq->m_full);
    return 0;
}

int my_receive(ShmQueue *t_myq, char *t_msg, int t_len) {
    down(&t_myq->m_full);
    down(&t_myq->m_mutex);

    // Remove item
    strncpy(t_msg, t_myq->m_data[t_myq->m_first_msg], MSG_LEN);
    t_myq->m_first_msg = (t_myq->m_first_msg + 1) % MSG_NUM;

    up(&t_myq->m_mutex);
    up(&t_myq->m_empty);
    return 0;
}

int my_send_z(ShmQueueZ *t_myq, char *t_msg, int t_len) {
    down(&t_myq->m_empty);
    down(&t_myq->m_mutex);

    // Insert item
    strncpy(t_myq->m_data[t_myq->m_last_msg], t_msg, MSG_LEN);
    t_myq->m_last_msg = (t_myq->m_last_msg + 1) % MSG_NUM;

    up(&t_myq->m_mutex);
    up(&t_myq->m_full);
    return 0;
}

int my_receive_z(ShmQueueZ *t_myq, char *t_msg, int t_len) {
    down(&t_myq->m_full);
    down(&t_myq->m_mutex);

    // Remove item
    strncpy(t_msg, t_myq->m_data[t_myq->m_first_msg], MSG_LEN);
    t_myq->m_first_msg = (t_myq->m_first_msg + 1) % MSG_NUM;

    up(&t_myq->m_mutex);
    up(&t_myq->m_empty);
    return 0;
}

void producer(int socketProducer) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        int read_bytes = read(socketProducer, buffer, BUFFER_SIZE);
        if (read_bytes <= 0) {
            cout << "Producer disconnected or error occurred." << endl;
            break;
        }
        buffer[read_bytes] = '\0';
        if(isFirstLetterBetweenAandL(buffer)){
            my_send(shmQueue, buffer, read_bytes);
        }else{
            my_send_z(shmQueueZ, buffer, read_bytes);
        }

        send(socketProducer, ACK, strlen(ACK), 0);
    }

    close(socketProducer);
}

void consumerA(int socketConsumer) {
    char buffer[BUFFER_SIZE] = {0};
    char ackBuf[3] = {0};

    while (true) {
        my_receive(shmQueue, buffer, BUFFER_SIZE);

        int sent_bytes = send(socketConsumer, buffer, strlen(buffer), 0);
        cout << sent_bytes << endl;
        if (sent_bytes <= 0) {
            cout << "Consumer disconnected or error occurred." << endl;
            break;
        }

        read(socketConsumer, ackBuf, 3);
    }

    close(socketConsumer);
}

void consumerZ(int socketConsumer) {
    char buffer[BUFFER_SIZE] = {0};
    char ackBuf[3] = {0};

    while (true) {
        my_receive_z(shmQueueZ, buffer, BUFFER_SIZE);

        int sent_bytes = send(socketConsumer, buffer, strlen(buffer), 0);
        cout << sent_bytes << endl;
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
            producer(socket);
        } else if (type == "CONSUMER") {
            if(!shmQueue->m_consumer_connected){
                shmQueue->m_consumer_connected = true;
                consumerA(socket);
            }else if(!shmQueueZ->m_consumer_connected){
                shmQueueZ->m_consumer_connected = true;
                consumerZ(socket);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize shared memory for ShmQueue
    shmQueue = (ShmQueue*)mmap(NULL, sizeof(ShmQueue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&shmQueue->m_full, 1, 0);
    sem_init(&shmQueue->m_empty, 1, MSG_NUM);
    sem_init(&shmQueue->m_mutex, 1, 1);
    shmQueue->m_first_msg = 0;
    shmQueue->m_last_msg = 0;
    shmQueue->m_consumer_connected = false;
    // Initialize shared memory for ShmQueueZ
    shmQueueZ = (ShmQueueZ*)mmap(NULL, sizeof(ShmQueueZ), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&shmQueueZ->m_full, 1, 0);
    sem_init(&shmQueueZ->m_empty, 1, MSG_NUM);
    sem_init(&shmQueueZ->m_mutex, 1, 1);
    shmQueueZ->m_first_msg = 0;
    shmQueueZ->m_last_msg = 0;
    shmQueueZ->m_consumer_connected = false;

    // Socket setup
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
        cout << "Waiting for connection" << endl;
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

    // Cleanup
    munmap(shmQueue, sizeof(ShmQueue));
    sem_destroy(&shmQueue->m_full);
    sem_destroy(&shmQueue->m_empty);
    sem_destroy(&shmQueue->m_mutex);

    return 0;
}
