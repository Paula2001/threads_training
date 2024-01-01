#include <iostream>
#include <string>
#include <vector>
#define BOARD_H 10
#define BOARD_V 10
#define BOARD_SYMBOL " [-] "
#define SNAKE_BODY_SYMBOL "  ~  "
#define SNAKE_HEAD " <:~ "
#define SNAKE_DEFAULT_SIZE 5
using namespace std;
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
        snakeBody.push_back({0, i});
    }

    return snakeBody;
}

vector<pair<int, int>> snake = createSnakeBody(SNAKE_DEFAULT_SIZE);

void moveSnake(pair<int, int> headPosition) {
    for (int i = 0; i < SNAKE_DEFAULT_SIZE; ++i) {
        int nextCell = i + 1;
        int cell = i;
        if(nextCell == SNAKE_DEFAULT_SIZE){
            snake[cell] = headPosition;
        }else{
            snake[cell] = snake[nextCell];
        }
    }
}

pair<int,int> getNextSnakeHeadMove(char movement) {
    pair<int,int> snakeHead = snake.back();
    // ? V , H
    switch (movement) {
        case 'u':{
            int v = snakeHead.first - 1;
            if (v < 0){
                v = 9;
            }
            return {v, snakeHead.second};
        }
        case 'd':{
            int v = snakeHead.first + 1;
            if(v > 9){
                v = 0;
            }
            return {v, snakeHead.second};
        }
        case 'r':
            return {snakeHead.first, snakeHead.second + 1};
        case 'l':
            return {snakeHead.first, snakeHead.second - 1};
        default:
            return snakeHead;
    }
}

string initBoard(pair<int, int> headPos) {
    moveSnake(headPos);
    pair<int, int> snakeHead = snake.back();
    string board;
    for (int i = 0; i < BOARD_V; ++i) {
        for(int j = 0; j < BOARD_H; ++j){
            // ? V, H
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
    char move ;
    while (move != 'c'){
        cout << "Insert the char \n";
        cin >> move;
        pair<int, int> newHead = getNextSnakeHeadMove(move);
        cout << initBoard(newHead);
    }
}