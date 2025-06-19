#include "Game.h"
#include <SFML/System.hpp>

int main() {
    Game game;
    sf::Clock clock;

    while (game.isWindowOpen()) {
        float deltaTime = clock.restart().asSeconds();
        game.handleInput();
        game.update(deltaTime);

        if (game.inMainMenu) {
            game.renderMainMenu();
        } else if (game.showRating) {
            game.renderRating();
        } else {
            game.renderGame();
        }
    }

    return 0;
}
