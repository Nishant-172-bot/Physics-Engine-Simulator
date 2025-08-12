#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <vector>
#include <sstream>
#include <iostream>

static constexpr float PROJECTILE_GRAVITY = 500.f;

class PhysicsCircle {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    bool settled;

    PhysicsCircle(float x, float y)
        : shape(10.f), velocity(0.f, 0.f), settled(false)
    {
        shape.setOrigin(10.f, 10.f);
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color::White);
    }

    float bottom() const { return shape.getPosition().y + shape.getRadius(); }
    void setBottom(float y) { shape.setPosition(shape.getPosition().x, y - shape.getRadius()); }
};

sf::VertexArray makeWave(const sf::RectangleShape& cont, float phase, sf::Color color) {
    const int P = 50;
    sf::VertexArray wave(sf::TriangleStrip, P * 2);
    float dx = cont.getSize().x / (P - 1);
    for (int i = 0; i < P; ++i) {
        float x = i * dx;
        float y = 6.f * std::sin(0.05f * x + phase);
        sf::Vector2f top = cont.getPosition() + sf::Vector2f(x, y + cont.getSize().y / 2.f);
        sf::Vector2f bot = cont.getPosition() + sf::Vector2f(x, cont.getSize().y);
        wave[i * 2].position = top;
        wave[i * 2 + 1].position = bot;
        wave[i * 2].color = wave[i * 2 + 1].color = color;
    }
    return wave;
}

void runViscositySimulation() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Viscosity Simulation");
    window.setFramerateLimit(60);

    sf::RectangleShape ground;
    std::vector<sf::RectangleShape> containers;
    std::vector<PhysicsCircle> balls;
    std::vector<sf::Text> labels;
    std::vector<float> phases;
    sf::Font font;
    bool isRunning = false;
    sf::Clock clock;
    static constexpr int COUNT = 5;
    std::vector<float> viscosityValues{ 5.f,8.f,15.f,50.f,30.f };
    std::vector<std::string> names{ "Water","Alcohol","Oil","Honey","Glycerine" };
    std::vector<sf::Color> colors{
        sf::Color(64,164,223,180), sf::Color(194,245,255,180),
        sf::Color(255,222,89,180), sf::Color(204,142,53,200), sf::Color(230,230,255,200)
    };

    float gh = 350.f;
    ground.setSize({ (float)window.getSize().x, gh });
    ground.setPosition(0.f, window.getSize().y - gh);
    ground.setFillColor(sf::Color::Black);

    if (!font.loadFromFile("OpenSans-Regular.ttf")) std::cerr << "Font load failed\n";

    float width = 150.f, height = 400.f, spacing = 100.f;
    float startX = (window.getSize().x - (COUNT * width + (COUNT - 1) * spacing)) / 2.f;
    float topY = window.getSize().y - gh - height;

    for (int i = 0; i < COUNT; ++i) {
        sf::RectangleShape cont({ width, height });
        cont.setPosition(startX + i * (width + spacing), topY);
        cont.setFillColor(sf::Color::Transparent);
        cont.setOutlineColor(sf::Color::White);
        cont.setOutlineThickness(2.f);
        containers.push_back(cont);

        float cx = cont.getPosition().x + width / 2.f;
        float cy = cont.getPosition().y + height / 2.f - 10.f;
        balls.emplace_back(cx, cy);

        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(20);
        label.setFillColor(sf::Color::White);
        float lx = cont.getPosition().x + 10.f;
        float ly = cont.getPosition().y + height + 5.f;
        label.setPosition(lx, ly);
        std::ostringstream ss;
        ss << names[i] << " - " << viscosityValues[i] << " mPa·s";
        label.setString(ss.str());
        labels.push_back(label);

        phases.push_back(0.f);
    }

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) isRunning = true;
                if (event.key.code == sf::Keyboard::R) {
                    isRunning = false;
                    for (int i = 0; i < COUNT; ++i) {
                        float cx = containers[i].getPosition().x + containers[i].getSize().x / 2.f;
                        float cy = containers[i].getPosition().y + containers[i].getSize().y / 2.f - 10.f;
                        balls[i].shape.setPosition(cx, cy);
                        balls[i].velocity = { 0.f,0.f };
                        balls[i].settled = false;
                    }
                }
            }
        }
        if (isRunning) {
            for (int i = 0; i < COUNT; ++i) {
                if (!balls[i].settled) {
                    float a = PROJECTILE_GRAVITY - viscosityValues[i] * balls[i].velocity.y;
                    balls[i].velocity.y += a * dt;
                    balls[i].shape.move(0.f, balls[i].velocity.y * dt);
                    float bottomY = containers[i].getPosition().y + containers[i].getSize().y - balls[i].shape.getRadius();
                    if (balls[i].bottom() >= bottomY) {
                        balls[i].setBottom(bottomY);
                        balls[i].settled = true;
                        balls[i].velocity = { 0.f,0.f };
                    }
                }
                phases[i] += dt * 2.f;
            }
        }
        window.clear(sf::Color::Black);
        window.draw(ground);
        for (int i = 0; i < COUNT; ++i) {
            window.draw(containers[i]);
            window.draw(makeWave(containers[i], phases[i], colors[i]));
            window.draw(balls[i].shape);
            window.draw(labels[i]);
        }
        window.display();
    }
}
       
