#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;

int main() {
    int pipefd[2];
    pid_t pid;
    char buf[64];
    const int mainBytesLength = 64;
    memset(buf, 0, sizeof(buf));

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Child process
        close(pipefd[1]); // Close unused write end

        while(true) {
            int len;
            if (read(pipefd[0], buf, sizeof(int)) <= 0) {
                break;
            }
            memcpy(&len, buf, sizeof(int));

            memset(buf, 0, sizeof(buf));
            if (read(pipefd[0], buf, len) <= 0) {
                break;
            }

            cout << buf << endl;
        }

        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    } else {  // Parent process
        string words[] = {
                "mango", "keyboard", "stream",
                "quantum", "galaxy", "festival",
                "harmony", "circuit", "voyage",
                "dinosaur"
        };

        close(pipefd[0]); // Close unused read end

        for (const auto &word : words) {
            int len = word.length();
            memset(buf, 0, sizeof(buf));
            memcpy(buf, &len, sizeof(int));
            memcpy(buf + sizeof(int), word.c_str(), len);
            write(pipefd[1], buf, sizeof(int) + len);
        }

        close(pipefd[1]); // Reader will see EOF
        wait(NULL); // Wait for child
        exit(EXIT_SUCCESS);
    }
}
