#include <iostream>
#include <string>
#include <vector>
#define BOARD_H 10
#define BOARD_V 10
#define BOARD_SYMBOL " [-] "
#define SNAKE_BODY_SYMBOL "  ~  "
#define SNAKE_HEAD " <:~ "
#define SNAKE_DEFAULT_SIZE 3
using namespace std;

// TODO : move the snake
// TODO : create snake

bool contains(const std::vector<std::pair<int, int>>& snakeBody, const std::pair<int, int>& target) {
    for (const auto& segment : snakeBody) {
        if (segment == target) {
            return true;
        }
    }
    return false;
}
vector<pair<int, int>> createSnakeBody(int snakeSize) {
    vector<pair<int, int>> snakeBody;

    for (int i = 0; i < snakeSize; ++i) {
        // Assuming a horizontal snake for example purposes
        snakeBody.push_back({0, i});
    }

    return snakeBody;
}
string initBoard() {
    vector<pair<int, int>> snake = createSnakeBody(3);
    pair<int, int> snakeHead = snake.back();
    string board;
    for (int i = 0; i < BOARD_V; ++i) {
        for(int j = 0; j < BOARD_H; ++j){
            pair<int, int> currentSqu = {i, j};
            if(contains(snake, currentSqu)){
                if (snakeHead == currentSqu){
                    board += SNAKE_HEAD;
                }else{
                    board += SNAKE_BODY_SYMBOL;
                }
            }else{
                board += BOARD_SYMBOL;
            }
        }
        board += "\n";
    }
    return board;
}

int main() {
    std::cout << initBoard();
}