#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUFFER_SIZE 5  // Size of the buffer

// Structure for shared memory
struct SharedMemory {
    int buffer[BUFFER_SIZE];
    int in, out;
};

int main() {
    int shm_fd;
    struct SharedMemory *shared_mem;

    // Create shared memory object
    shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct SharedMemory));
    shared_mem = (struct SharedMemory *)mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initialize buffer indices
    shared_mem->in = 0;
    shared_mem->out = 0;

    // Create named semaphores
    sem_t *empty = sem_open("/empty_sem", O_CREAT, 0644, BUFFER_SIZE);
    sem_t *full = sem_open("/full_sem", O_CREAT, 0644, 0);

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process - Consumer
        for (int i = 0; i < 10; i++) {
            sem_wait(full);
            int item = shared_mem->buffer[shared_mem->out];
            shared_mem->out = (shared_mem->out + 1) % BUFFER_SIZE;
            printf("Consumed: %d\n", item);
            sem_post(empty);
        }
        exit(EXIT_SUCCESS);
    } else {
        // Parent process - Producer
        for (int i = 0; i < 10; i++) {
            sleep(1); // Simulate time to produce item
            sem_wait(empty);
            shared_mem->buffer[shared_mem->in] = i;
            shared_mem->in = (shared_mem->in + 1) % BUFFER_SIZE;
            printf("Produced: %d\n", i);
            sem_post(full);
        }
        wait(NULL); // Wait for child process to finish

        // Cleanup
        shm_unlink("/my_shm");
        sem_close(empty);
        sem_close(full);
        sem_unlink("/empty_sem");
        sem_unlink("/full_sem");
    }

    return 0;
}
