#pragma once
#include <string>

struct GameResult {
    std::string timestamp;
    int score;
    std::string result; // "ПОБЕДА" или "ПОРАЖЕНИЕ"
};
