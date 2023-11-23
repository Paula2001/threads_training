#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/wait.h> // Include for wait
#include <cstdlib>
using namespace  std;

int main() {
    int pipefd[2];
    pid_t pid;
    char buf[1];
    int mainBytesLength = 64;
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

        int num = 0;
        // Read from pipe
        while(read(pipefd[0], buf, sizeof(buf))) {
            num = atoi(buf);
            string word = "";
            for(int i = 0; i < num;i++){
                read(pipefd[0], buf, sizeof(buf));
                word += buf;
                std::cout << "Child received: " << buf << std::endl;
            }
            cout << "=======================" << word << endl;
        }

        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    } else {  // Parent process
        string x = "";
        string words[] = {
                "mango", "keyboard", "stream",
                "quantum", "galaxy", "festival",
                "harmony", "circuit", "voyage",
                "dinosaur"
        };
        for (const auto & word : words) {
            if(x.length() + word.length() + 1 >= mainBytesLength) {
                break;
            }
            x += to_string(word.length()) + word;
        }

        close(pipefd[0]); // Close unused read end

        // Write to pipe
        write(pipefd[1], x.c_str(), x.length());
        close(pipefd[1]); // Reader will see EOF

        wait(NULL); // Wait for child
        exit(EXIT_SUCCESS);
    }
}