#include "Game.h"
#include <iostream>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main() {
    try {
        Game game(SCREEN_WIDTH, SCREEN_HEIGHT);
        game.Init();
        game.Run();
    } catch (const std::exception& e) {
        std::cerr << "\n--- An error was caught by try-catch ---" << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "------------------------------------------" << std::endl;
    }

    // Эта пауза сработает всегда, даже если программа упадет без исключения
    std::cout << "\nProgram finished. Press Enter to exit..." << std::endl;
    std::cin.get(); 

    return 0;
}