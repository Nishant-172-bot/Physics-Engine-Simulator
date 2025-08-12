#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>

// ANSI Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"

static constexpr float PI = 3.14159265f;
static constexpr float TIME_SCALE = 5.f;
static constexpr float ORBIT_GRAVITY = 0.1f;
static constexpr float PROJECTILE_GRAVITY = 500.f;

class Simulation {
public:
    virtual void run() = 0;
    virtual ~Simulation() = default;
};

void runOrbitSimulation();
void runProjectileSimulation();
void runCollisionSimulation();
void runViscositySimulation();

void printMenu() {
    std::cout << BOLD << CYAN;
    std::cout << "\n========================================\n";
    std::cout << "         Physics Simulation Hub     \n";
    std::cout << "========================================" << RESET << "\n";
    std::cout << YELLOW << " Select Simulation Mode:\n" << RESET;
    std::cout << GREEN
        << "  [1] Planetary Orbit\n"
        << "  [2] Projectile Motion\n"
        << "  [3] Collision Simulation\n"
        << "  [4] Viscosity Simulation\n"
        << RED << "  [5] Quit\n" << RESET;
    std::cout << MAGENTA << "----------------------------------------\n" << RESET;
    std::cout << CYAN << " Enter choice: " << RESET;
}

int main() {
    while (true) {
        printMenu();
        int choice;
        if (!(std::cin >> choice)) break;
        switch (choice) {
        case 1:
            runOrbitSimulation(); break;
        case 2:
            runProjectileSimulation(); break;
        case 3:
            runCollisionSimulation(); break;
        case 4:
            runViscositySimulation(); break;
        case 5:
            std::cout << GREEN << "\nExiting... Have a great day! 🚀\n" << RESET; return 0;
        default:
            std::cout << RED << "\n❌ Invalid choice, please try again.\n" << RESET;
        }
    }
    return 0;
}
