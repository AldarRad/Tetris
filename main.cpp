#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

const int CELL_SIZE = 30;
const int FIELD_WIDTH = 10;
const int FIELD_HEIGHT = 20;
const int WINDOW_WIDTH = FIELD_WIDTH * CELL_SIZE + 200;
const int WINDOW_HEIGHT = FIELD_HEIGHT * CELL_SIZE;

struct Tetromino {
    std::vector<std::vector<int>> shape;
    sf::Color color;
    int x, y;
};

struct GameResult {
    std::string timestamp;
    int score;
    std::string result; // "WIN" или "LOSE"
};

class Game {
private:
    sf::RenderWindow window;
    std::vector<std::vector<sf::Color>> field;
    Tetromino currentPiece;
    float elapsedTime;
    float delay;
    int score;
    bool isGameOver;
    bool isGameWon;
    std::vector<GameResult> bestResults;
    bool isGameFinished;
    bool showResults;
    sf::Font font;

    std::vector<Tetromino> pieces = {
        { { {1, 1, 1, 1} }, sf::Color::Cyan, 0, 0 },
        { { {1, 1}, {1, 1} }, sf::Color::Yellow, 0, 0 },
        { { {0, 1, 0}, {1, 1, 1} }, sf::Color::Magenta, 0, 0 },
        { { {1, 1, 0}, {0, 1, 1} }, sf::Color::Red, 0, 0 },
        { { {0, 1, 1}, {1, 1, 0} }, sf::Color::Green, 0, 0 },
        { { {1, 0, 0}, {1, 1, 1} }, sf::Color::Blue, 0, 0 },
        { { {0, 0, 1}, {1, 1, 1} }, sf::Color(255, 165, 0), 0, 0 }
    };

    void loadResults() {
        bestResults.clear();
        std::ifstream file("tetris_results.txt");
        if (file.is_open()) {
            GameResult result;
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss(line);
                if (iss >> result.timestamp >> result.score >> result.result) {
                    bestResults.push_back(result);
                }
            }
            file.close();
        }
        // Sort by score descending
        std::sort(bestResults.begin(), bestResults.end(), 
            [](const GameResult& a, const GameResult& b) {
                return a.score > b.score;
            });
    }

    void saveResults() {
        std::ofstream file("tetris_results.txt", std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            for (const auto& result : bestResults) {
                file << result.timestamp << " " 
                     << result.score << " "
                     << result.result << "\n";
            }
            file.close();
        } else {
            std::cerr << "Error: Unable to save results to file!" << std::endl;
        }
    }

    void addResult(int score, const std::string& result) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &in_time_t);
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        GameResult gameResult;
        gameResult.timestamp = ss.str();
        gameResult.score = score;
        gameResult.result = result;

        bestResults.push_back(gameResult);
        // Sort by score descending
        std::sort(bestResults.begin(), bestResults.end(), 
            [](const GameResult& a, const GameResult& b) {
                return a.score > b.score;
            });
        
        if (bestResults.size() > 10) {
            bestResults.pop_back();
        }
        saveResults();
    }

    void finishGame(const std::string& result) {
        if (!isGameFinished) {
            isGameFinished = true;
            addResult(score, result);
        }
    }

public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Tetris"),
             field(FIELD_HEIGHT, std::vector<sf::Color>(FIELD_WIDTH, sf::Color::Black)),
             elapsedTime(0), delay(0.5f), score(0), 
             isGameOver(false), isGameWon(false), isGameFinished(false),
             showResults(false) {
        // Проверяем и создаем файл результатов, если его нет
        std::ifstream checkFile("tetris_results.txt");
        if (!checkFile.good()) {
            std::ofstream createFile("tetris_results.txt");
            createFile.close();
        }
        if (!font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf")) {
            std::cerr << "Error loading font!" << std::endl;
        }
        loadResults();
        resetGame();
    }

    void resetGame() {
        field = std::vector<std::vector<sf::Color>>(FIELD_HEIGHT, std::vector<sf::Color>(FIELD_WIDTH, sf::Color::Black));
        elapsedTime = 0;
        delay = 0.5f;
        score = 0;
        isGameOver = false;
        isGameWon = false;
        isGameFinished = false;
        showResults = false;
        spawnPiece();
    }

    void spawnPiece() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, pieces.size() - 1);
        
        currentPiece = pieces[dist(gen)];
        currentPiece.x = FIELD_WIDTH / 2 - currentPiece.shape[0].size() / 2;
        currentPiece.y = 0;
        
        // Проверка на проигрыш (не можем разместить новую фигуру)
        if (!isValidPosition()) {
            isGameOver = true;
            finishGame("LOSE");
        }

        // Проверка на победу (фигура выходит за верхнюю границу)
        for (size_t i = 0; i < currentPiece.shape.size(); ++i) {
            for (size_t j = 0; j < currentPiece.shape[i].size(); ++j) {
                if (currentPiece.shape[i][j] == 0) continue;
                
                int newY = currentPiece.y + i;
                if (newY < 0) {
                    isGameWon = true;
                    finishGame("WIN");
                    return;
                }
            }
        }
    }

    bool isValidPosition() {
        for (size_t i = 0; i < currentPiece.shape.size(); ++i) {
            for (size_t j = 0; j < currentPiece.shape[i].size(); ++j) {
                if (currentPiece.shape[i][j] == 0) continue;
                
                int newX = currentPiece.x + j;
                int newY = currentPiece.y + i;
                
                if (newX < 0 || newX >= FIELD_WIDTH || newY >= FIELD_HEIGHT) {
                    return false;
                }
                
                if (newY >= 0 && field[newY][newX] != sf::Color::Black) {
                    return false;
                }
            }
        }
        return true;
    }

    void rotatePiece() {
        if (isGameOver || isGameWon) return;
        
        std::vector<std::vector<int>> rotated(currentPiece.shape[0].size(), 
                                     std::vector<int>(currentPiece.shape.size()));
        
        for (size_t i = 0; i < currentPiece.shape.size(); ++i) {
            for (size_t j = 0; j < currentPiece.shape[i].size(); ++j) {
                rotated[j][currentPiece.shape.size() - 1 - i] = currentPiece.shape[i][j];
            }
        }
        
        auto oldShape = currentPiece.shape;
        currentPiece.shape = rotated;
        
        if (!isValidPosition()) {
            currentPiece.shape = oldShape;
        }
    }

    void lockPiece() {
        for (size_t i = 0; i < currentPiece.shape.size(); ++i) {
            for (size_t j = 0; j < currentPiece.shape[i].size(); ++j) {
                if (currentPiece.shape[i][j] == 0) continue;
                
                int fieldX = currentPiece.x + j;
                int fieldY = currentPiece.y + i;
                
                if (fieldY >= 0) {
                    field[fieldY][fieldX] = currentPiece.color;
                }
            }
        }
        
        checkLines();
        spawnPiece();
    }

    void checkLines() {
        for (int i = FIELD_HEIGHT - 1; i >= 0; --i) {
            bool lineComplete = true;
            for (int j = 0; j < FIELD_WIDTH; ++j) {
                if (field[i][j] == sf::Color::Black) {
                    lineComplete = false;
                    break;
                }
            }
            
            if (lineComplete) {
                field.erase(field.begin() + i);
                field.insert(field.begin(), std::vector<sf::Color>(FIELD_WIDTH, sf::Color::Black));
                score += 100;
                delay *= 0.95f;
            }
        }
    }

    void handleInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Tab) {
                    showResults = !showResults;
                }
                else if (isGameOver || isGameWon) {
                    if (event.key.code == sf::Keyboard::R) {
                        resetGame();
                    }
                }
                else {
                    if (event.key.code == sf::Keyboard::Left) {
                        currentPiece.x--;
                        if (!isValidPosition()) currentPiece.x++;
                    }
                    else if (event.key.code == sf::Keyboard::Right) {
                        currentPiece.x++;
                        if (!isValidPosition()) currentPiece.x--;
                    }
                    else if (event.key.code == sf::Keyboard::Down) {
                        currentPiece.y++;
                        if (!isValidPosition()) {
                            currentPiece.y--;
                            lockPiece();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Up) {
                        rotatePiece();
                    }
                    else if (event.key.code == sf::Keyboard::Space) {
                        while (isValidPosition()) {
                            currentPiece.y++;
                        }
                        currentPiece.y--;
                        lockPiece();
                    }
                }
            }
        }
    }

    void update(float deltaTime) {
        if (isGameOver || isGameWon) return;
        
        elapsedTime += deltaTime;
        if (elapsedTime >= delay) {
            currentPiece.y++;
            if (!isValidPosition()) {
                currentPiece.y--;
                lockPiece();
            }
            elapsedTime = 0;
        }
    }

    void render() {
        window.clear(sf::Color::Black);
        
        // Рисуем игровое поле
        for (int i = 0; i < FIELD_HEIGHT; ++i) {
            for (int j = 0; j < FIELD_WIDTH; ++j) {
                if (field[i][j] != sf::Color::Black) {
                    sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
                    cell.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                    cell.setFillColor(field[i][j]);
                    window.draw(cell);
                }
            }
        }
        
        // Рисуем текущую фигуру (если игра не завершена победой)
        if (!isGameWon) {
            for (size_t i = 0; i < currentPiece.shape.size(); ++i) {
                for (size_t j = 0; j < currentPiece.shape[i].size(); ++j) {
                    if (currentPiece.shape[i][j] != 0) {
                        int x = (currentPiece.x + j) * CELL_SIZE;
                        int y = (currentPiece.y + i) * CELL_SIZE;
                        
                        if (y >= 0) {
                            sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
                            cell.setPosition(x, y);
                            cell.setFillColor(currentPiece.color);
                            window.draw(cell);
                        }
                    }
                }
            }
        }
        
        // Рисуем интерфейс
        if (font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf")) {
            // Текущие показатели
            sf::Text scoreText("Score: " + std::to_string(score), font, 20);
            scoreText.setPosition(FIELD_WIDTH * CELL_SIZE + 10, 20);
            scoreText.setFillColor(sf::Color::White);
            window.draw(scoreText);
            
            // Сообщения о завершении игры
            if (isGameOver) {
                sf::Text gameOverText("GAME OVER\nScore: " + std::to_string(score) + 
                                     "\nPress R to restart", font, 24);
                gameOverText.setPosition(FIELD_WIDTH * CELL_SIZE + 10, 100);
                gameOverText.setFillColor(sf::Color::Red);
                window.draw(gameOverText);
            }
            
            if (isGameWon) {
                sf::Text winText("YOU WIN!\nScore: " + std::to_string(score) + 
                                "\nPress R to restart", font, 24);
                winText.setPosition(FIELD_WIDTH * CELL_SIZE + 10, 100);
                winText.setFillColor(sf::Color::Green);
                window.draw(winText);
            }
            
            // Окно с результатами при нажатии Tab
            if (showResults) {
                sf::RectangleShape resultsBackground(sf::Vector2f(WINDOW_WIDTH - 50, WINDOW_HEIGHT - 50));
                resultsBackground.setPosition(25, 25);
                resultsBackground.setFillColor(sf::Color(50, 50, 50, 230));
                window.draw(resultsBackground);
                
                sf::Text resultsTitle("Best Results (Press Tab to close):", font, 24);
                resultsTitle.setPosition(50, 30);
                resultsTitle.setFillColor(sf::Color::Yellow);
                window.draw(resultsTitle);
                
                for (size_t i = 0; i < bestResults.size(); ++i) {
                    sf::Text resultText(
                        std::to_string(i+1) + ". " + bestResults[i].timestamp + " - " +
                        std::to_string(bestResults[i].score) + " - " +
                        bestResults[i].result,
                        font, 18);
                    resultText.setPosition(50, 70 + i * 30);
                    resultText.setFillColor(sf::Color::White);
                    window.draw(resultText);
                }
            }
        }
        
        window.display();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds();
            handleInput();
            update(deltaTime);
            render();
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}