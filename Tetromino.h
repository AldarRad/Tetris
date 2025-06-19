#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Tetromino {
    std::vector<std::vector<int>> shape;
    sf::Color color;
    int x, y;
};
