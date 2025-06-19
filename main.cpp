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
const int WINDOW_WIDTH = FIELD_WIDTH * CELL_SIZE + 300;
const int WINDOW_HEIGHT = FIELD_HEIGHT * CELL_SIZE;

struct Tetromino {
    std::vector<std::vector<int>> shape;
    sf::Color color;
    int x, y;
};

struct GameResult {
    std::string timestamp;
    int score;
    std::string result; // "ПОБЕДА" или "ПОРАЖЕНИЕ"
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
    bool isPaused;
    bool inMainMenu;
    bool showRating;
    std::vector<std::string> menuItems;
    int selectedMenuItem;
    sf::RectangleShape restartButton;
    sf::Text restartButtonText;
    sf::RectangleShape fieldBorder;

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
        // Сортировка по очкам (по убыванию)
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
            std::cerr << "Ошибка: Не удалось сохранить результаты в файл!" << std::endl;
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
        // Сортировка по очкам (по убыванию)
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
            inMainMenu = true;
        }
    }

    void initRestartButton() {
        restartButtonText.setString("Restart (R)");

    }

    void initFieldBorder() {
        fieldBorder.setSize(sf::Vector2f(FIELD_WIDTH * CELL_SIZE + 2, FIELD_HEIGHT * CELL_SIZE + 2));
        fieldBorder.setPosition(-1, -1);
        fieldBorder.setFillColor(sf::Color::Transparent);
        fieldBorder.setOutlineThickness(2);
        fieldBorder.setOutlineColor(sf::Color::White);
    }

public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Tetris"),
             field(FIELD_HEIGHT, std::vector<sf::Color>(FIELD_WIDTH, sf::Color::Black)),
             elapsedTime(0), delay(0.5f), score(0), 
             isGameOver(false), isGameWon(false), isGameFinished(false),
             showResults(false), isPaused(false), inMainMenu(true),
             showRating(false), selectedMenuItem(0) {
        
        menuItems = {"Start Game", "View Rating", "Exit"};
        
        // Проверяем и создаем файл результатов, если его нет
        std::ifstream checkFile("tetris_results.txt");
        if (!checkFile.good()) {
            std::ofstream createFile("tetris_results.txt");
            createFile.close();
        }
        if (!font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf")) {
            std::cerr << "Ошибка загрузки шрифта!" << std::endl;
        }

        initRestartButton();
        initFieldBorder();
        loadResults();
    }

    void resetGame() {
        field = std::vector<std::vector<sf::Color>>(FIELD_HEIGHT, std::vector<sf::Color>(FIELD_WIDTH, sf::Color::Black));
        elapsedTime = 0;
        delay = 0.5f;
        score = 0;
        isGameOver = false;
        isGameWon = false;
        isGameFinished = false;
        isPaused = false;
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
        if (isGameOver || isGameWon || isPaused) return;
        
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
                if (inMainMenu) {
                    if (event.key.code == sf::Keyboard::Up) {
                        selectedMenuItem = (selectedMenuItem - 1 + menuItems.size()) % menuItems.size();
                    }
                    else if (event.key.code == sf::Keyboard::Down) {
                        selectedMenuItem = (selectedMenuItem + 1) % menuItems.size();
                    }
                    else if (event.key.code == sf::Keyboard::Enter) {
                        if (selectedMenuItem == 0) { // Начать игру
                            inMainMenu = false;
                            resetGame();
                        }
                        else if (selectedMenuItem == 1) { // Рейтинг
                            showRating = true;
                        }
                        else if (selectedMenuItem == 2) { // Выход
                            window.close();
                        }
                    }
                }
                else if (showRating) {
                    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Enter) {
                        showRating = false;
                    }
                }
                else if (isGameOver || isGameWon) {
                    if (event.key.code == sf::Keyboard::R) {
                        inMainMenu = true;
                    }
                    else if (event.key.code == sf::Keyboard::Escape) {
                        inMainMenu = true;
                    }
                }
                else {
                    if (event.key.code == sf::Keyboard::Escape) {
                        isPaused = !isPaused;
                    }
                    else if (event.key.code == sf::Keyboard::Tab) {
                        showResults = !showResults;
                    }
                    else if (event.key.code == sf::Keyboard::R) {
                        resetGame();
                    }
                    else if (!isPaused) {
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
            
            // Обработка клика по кнопке "Заново"
            if (event.type == sf::Event::MouseButtonPressed && 
                event.mouseButton.button == sf::Mouse::Left &&
                !inMainMenu && !showRating) {
                
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (restartButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    resetGame();
                }
            }
        }
    }

    void update(float deltaTime) {
        if (inMainMenu || isGameOver || isGameWon || isPaused) return;
        
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

    void renderMainMenu() {
        window.clear(sf::Color::Black);
        
        sf::Text title("TETRIS", font, 50);
        title.setPosition(WINDOW_WIDTH / 2 - title.getGlobalBounds().width / 2, 50);
        title.setFillColor(sf::Color::White);
        window.draw(title);
        
        for (size_t i = 0; i < menuItems.size(); ++i) {
            sf::Text item(menuItems[i], font, 30);
            item.setPosition(WINDOW_WIDTH / 2 - item.getGlobalBounds().width / 2, 150 + i * 50);
            
            if (i == selectedMenuItem) {
                item.setFillColor(sf::Color::Yellow);
                item.setStyle(sf::Text::Bold);
            } else {
                item.setFillColor(sf::Color::White);
            }
            
            window.draw(item);
        }
        
        sf::Text hint("Use UP/DOWN arrows to select, ENTER to confirm", font, 16);
        hint.setPosition(WINDOW_WIDTH / 2 - hint.getGlobalBounds().width / 2, WINDOW_HEIGHT - 50);
        hint.setFillColor(sf::Color(150, 150, 150));
        window.draw(hint);
        
        window.display();
    }

    void renderRating() {
        window.clear(sf::Color::Black);
        
        sf::Text title("TOP 10 RESULTS", font, 40);
        title.setPosition(WINDOW_WIDTH / 2 - title.getGlobalBounds().width / 2, 30);
        title.setFillColor(sf::Color::Yellow);
        window.draw(title);
        
        if (bestResults.empty()) {
            sf::Text noResults("No results yet!", font, 30);
            noResults.setPosition(WINDOW_WIDTH / 2 - noResults.getGlobalBounds().width / 2, 150);
            noResults.setFillColor(sf::Color::White);
            window.draw(noResults);
        } else {
            for (size_t i = 0; i < bestResults.size(); ++i) {
                std::string resultStr = std::to_string(i+1) + ". " + 
                                      bestResults[i].timestamp + " - " +
                                      std::to_string(bestResults[i].score) + " - " +
                                      bestResults[i].result;
                
                sf::Text resultText(resultStr, font, 20);
                resultText.setPosition(50, 100 + i * 30);
                resultText.setFillColor(sf::Color::White);
                window.draw(resultText);
            }
        }
        
        sf::Text hint("Press ESC or ENTER to return", font, 20);
        hint.setPosition(WINDOW_WIDTH / 2 - hint.getGlobalBounds().width / 2, WINDOW_HEIGHT - 50);
        hint.setFillColor(sf::Color(150, 150, 150));
        window.draw(hint);
        
        window.display();
    }

    void renderGame() {
        window.clear(sf::Color::Black);
        
        // Рисуем границу игрового поля
        window.draw(fieldBorder);
        
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
            scoreText.setPosition(FIELD_WIDTH * CELL_SIZE + 20, 20);
            scoreText.setFillColor(sf::Color::White);
            window.draw(scoreText);
            
            // Кнопка "Заново"
            window.draw(restartButton);
            window.draw(restartButtonText);
            
            // Сообщение о паузе
            if (isPaused) {
                sf::Text pauseText("PAUSED\nPress ESC to continue", font, 30);
                pauseText.setPosition(FIELD_WIDTH * CELL_SIZE + 20, 100);
                pauseText.setFillColor(sf::Color::Yellow);
                window.draw(pauseText);
            }
            
            // Сообщения о завершении игры
            if (isGameOver) {
                sf::Text gameOverText("GAME OVER\nScore: " + std::to_string(score) + 
                                     "\nPress R to return to menu", font, 24);
                gameOverText.setPosition(FIELD_WIDTH * CELL_SIZE + 20, 100);
                gameOverText.setFillColor(sf::Color::Red);
                window.draw(gameOverText);
            }
            
            if (isGameWon) {
                sf::Text winText("YOU WIN!\nScore: " + std::to_string(score) + 
                                "\nPress R to return to menu", font, 24);
                winText.setPosition(FIELD_WIDTH * CELL_SIZE + 20, 100);
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
            
            // Подсказки управления
            sf::Text controls("Controls:\n"
                             "Left/Right: Move\n"
                             "Up: Rotate\n"
                             "Down: Drop faster\n"
                             "Space: Instant drop\n"
                             "ESC: Pause\n"
                             "Tab: Show results\n"
                             "R: Restart", font, 16);
            controls.setPosition(FIELD_WIDTH * CELL_SIZE + 20, WINDOW_HEIGHT - 180);
            controls.setFillColor(sf::Color(150, 150, 150));
            window.draw(controls);
        }
        
        window.display();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds();
            handleInput();
            
            if (inMainMenu) {
                renderMainMenu();
            } 
            else if (showRating) {
                renderRating();
            }
            else {
                update(deltaTime);
                renderGame();
            }
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}