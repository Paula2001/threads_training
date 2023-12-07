#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PORT 9130
#define WIDTH 20
#define HEIGHT 10
#define MAX_SNAKE_SIZE WIDTH * HEIGHT

// Directions
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

// Snake Game Structures and Functions
typedef struct {
    int x, y;
} Point;

typedef struct {
    Point body[MAX_SNAKE_SIZE];
    int size;
    int dir;
} Snake;

void initBoard(char board[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = ' ';
        }
    }
}

void printBoard(char board[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c ", board[y][x]);
        }
        printf("\n");
    }
}

void initSnake(Snake *snake) {
    snake->body[0].x = WIDTH / 2;
    snake->body[0].y = HEIGHT / 2;
    snake->size = 1;
    snake->dir = LEFT;
}

void moveSnake(Snake *snake, char board[HEIGHT][WIDTH], Point *prevHead) {
    *prevHead = snake->body[0];

    switch (snake->dir) {
        case UP:    prevHead->y--; break;
        case DOWN:  prevHead->y++; break;
        case LEFT:  prevHead->x--; break;
        case RIGHT: prevHead->x++; break;
    }

    for (int i = snake->size; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }

    snake->body[0] = *prevHead;
}

int updateBoard(Snake *snake, char board[HEIGHT][WIDTH], Point prevHead) {
    if (prevHead.x >= 0 && prevHead.x < WIDTH && prevHead.y >= 0 && prevHead.y < HEIGHT) {
        board[prevHead.y][prevHead.x] = 'X';
    }

    Point head = snake->body[0];
    if (head.x < 0 || head.x >= WIDTH || head.y < 0 || head.y >= HEIGHT) {
        return 1;
    }

    for (int i = 1; i < snake->size; i++) {
        if (head.x == snake->body[i].x && head.y == snake->body[i].y) {
            return 1;
        }
    }

    for (int i = 0; i <= snake->size; i++) {
        Point p = snake->body[i];
        if (i == 0) {
            board[p.y][p.x] = 'O';
        } else {
            board[p.y][p.x] = 'X';
        }
    }

    return 0;
}

void changeDirection(Snake *snake, const char *command) {
    if (strcmp(command, "UP") == 0) {
        snake->dir = UP;
    } else if (strcmp(command, "DOWN") == 0) {
        snake->dir = DOWN;
    } else if (strcmp(command, "LEFT") == 0) {
        snake->dir = LEFT;
    } else if (strcmp(command, "RIGHT") == 0) {
        snake->dir = RIGHT;
    }
}

// Server-specific Functions
void handleConnection(int socket) {
    char buffer[1024] = {0};
    char board[HEIGHT][WIDTH];
    Snake snake;
    Point prevHead = {0, 0};
    int gameOver = 0;

    initBoard(board);
    initSnake(&snake);
    updateBoard(&snake, board, prevHead);

    while (!gameOver) {
        int read_bytes = read(socket, buffer, 1024);
        if (read_bytes <= 0) {
            std::cout << "Client disconnected or error occurred." << std::endl;
            break;
        }

        buffer[read_bytes] = '\0';

        if (strcmp(buffer, "EXIT") == 0) {
            break;
        } else {
            changeDirection(&snake, buffer);
            moveSnake(&snake, board, &prevHead);
            gameOver = updateBoard(&snake, board, prevHead);
        }

        // Convert the game board to a string
        std::string boardString;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                boardString.push_back(board[y][x]);
                boardString.push_back(' ');
            }
            boardString.push_back('\n');
        }

        // Send the board string to the client
        send(socket, boardString.c_str(), boardString.length(), 0);
    }
}


int main() {
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
        std::cout << "Waiting for connection" << std::endl;
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pid_t child_pid = fork();
        if (child_pid == 0) {
            handleConnection(new_socket);
            close(new_socket);
            exit(0);
        } else if (child_pid > 0) {
            close(new_socket);
        } else {
            perror("fork");
        }
    }

    close(server_fd);
    return 0;
}
