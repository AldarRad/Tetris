#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Tetromino.h"
#include "GameResult.h"

const int CELL_SIZE = 30;
const int FIELD_WIDTH = 10;
const int FIELD_HEIGHT = 20;
const int WINDOW_WIDTH = FIELD_WIDTH * CELL_SIZE + 300;
const int WINDOW_HEIGHT = FIELD_HEIGHT * CELL_SIZE;

class Game {
private:
    sf::RenderWindow window;
    std::vector<std::vector<sf::Color>> field;
    Tetromino currentPiece;
    float elapsedTime;
    float delay;
    int score;
    bool isGameOver, isGameWon, isGameFinished, showResults;
    bool isPaused;
    std::vector<GameResult> bestResults;
    std::vector<std::string> menuItems;
    int selectedMenuItem;

    sf::Font font;
    sf::RectangleShape restartButton;
    sf::Text restartButtonText;
    sf::RectangleShape fieldBorder;

    std::vector<Tetromino> pieces;

    void loadResults();
    void saveResults();
    void addResult(int score, const std::string& result);
    void finishGame(const std::string& result);
    void initRestartButton();
    void initFieldBorder();
    void spawnPiece();
    bool isValidPosition();
    void rotatePiece();
    void lockPiece();
    void checkLines();

public:
    bool inMainMenu, showRating;
    Game();
    void resetGame();
    void handleInput();
    void update(float deltaTime);
    void renderMainMenu();
    void renderRating();
    void renderGame();
    bool isWindowOpen() const;
};
