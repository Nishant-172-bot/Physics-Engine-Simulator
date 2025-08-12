#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <vector>
#include <random>

struct Ball {
    sf::CircleShape shape;
    sf::Vector2f   velocity = { 0.f, 0.f };
    float          radius;
    Ball(float r, sf::Vector2f pos) : radius(r) {
        shape.setRadius(radius);
        shape.setOrigin(radius, radius);
        shape.setPosition(pos);
        shape.setFillColor({ 100, 200, 250 });
    }
};

void runCollisionSimulation() {
    const sf::Vector2u windowSize{ 800, 600 };

    sf::RenderWindow window(
        sf::VideoMode(windowSize.x, windowSize.y),
        "Container Collision"
    );
    window.setFramerateLimit(60);

    // Legend
    sf::Font font;
    font.loadFromFile("OpenSans-Regular.ttf");
    sf::Text legend("Drag the ball to throw it!", font, 18);
    legend.setFillColor(sf::Color::White);
    legend.setPosition(60.f, 20.f);

    // Container bounds
    sf::FloatRect bounds(50.f, 50.f,
        windowSize.x - 100.f,
        windowSize.y - 100.f);
    sf::RectangleShape containerShape;
    containerShape.setPosition(bounds.left, bounds.top);
    containerShape.setSize({ bounds.width, bounds.height });
    containerShape.setFillColor(sf::Color::Transparent);
    containerShape.setOutlineColor(sf::Color::White);
    containerShape.setOutlineThickness(2.f);

    // Random placement
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> ux(
        bounds.left + 20.f, bounds.left + bounds.width - 20.f
    );
    std::uniform_real_distribution<float> uy(
        bounds.top + 20.f, bounds.top + bounds.height - 20.f
    );

    std::vector<Ball> balls;
    balls.reserve(10);
    for (int i = 0; i < 10; ++i) {
        balls.emplace_back(15.f, sf::Vector2f(ux(rng), uy(rng)));
    }

    // Drag state
    bool dragging = false;
    int dragIndex = -1;
    sf::Vector2f dragStart;
    sf::Clock clock;

    const float restitution = 0.8f;
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
            if (e.type == sf::Event::KeyPressed &&
                e.key.code == sf::Keyboard::Space) {
                for (auto& b : balls)
                    b.velocity = { 0.f, 0.f };
                dragging = false;
            }
            if (e.type == sf::Event::MouseButtonPressed &&
                e.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f m = window.mapPixelToCoords(
                    { e.mouseButton.x, e.mouseButton.y }
                );
                for (int i = 0; i < (int)balls.size(); ++i) {
                    float d = std::hypot(
                        m.x - balls[i].shape.getPosition().x,
                        m.y - balls[i].shape.getPosition().y
                    );
                    if (d <= balls[i].radius) {
                        dragging = true;
                        dragIndex = i;
                        dragStart = m;
                        balls[i].velocity = { 0.f, 0.f };
                        break;
                    }
                }
            }
            if (e.type == sf::Event::MouseButtonReleased &&
                e.mouseButton.button == sf::Mouse::Left &&
                dragging) {
                sf::Vector2f m = window.mapPixelToCoords(
                    { e.mouseButton.x, e.mouseButton.y }
                );
                balls[dragIndex].velocity = (m - dragStart) * 5.f;
                dragging = false;
            }
        }

        float dt = clock.restart().asSeconds();

        // Move & wall collision
        for (auto& b : balls) {
            b.shape.move(b.velocity * dt);
            auto p = b.shape.getPosition();
            if (p.x - b.radius < bounds.left) {
                b.shape.setPosition(bounds.left + b.radius, p.y);
                b.velocity.x *= -restitution;
            }
            if (p.x + b.radius > bounds.left + bounds.width) {
                b.shape.setPosition(
                    bounds.left + bounds.width - b.radius, p.y
                );
                b.velocity.x *= -restitution;
            }
            if (p.y - b.radius < bounds.top) {
                b.shape.setPosition(p.x, bounds.top + b.radius);
                b.velocity.y *= -restitution;
            }
            if (p.y + b.radius > bounds.top + bounds.height) {
                b.shape.setPosition(
                    p.x, bounds.top + bounds.height - b.radius
                );
                b.velocity.y *= -restitution;
            }
        }

        // Ball-ball collisions (inelastic)
        for (int i = 0; i < (int)balls.size(); ++i) {
            for (int j = i + 1; j < (int)balls.size(); ++j) {
                auto& A = balls[i], & B = balls[j];
                sf::Vector2f d = B.shape.getPosition() - A.shape.getPosition();
                float dist = std::hypot(d.x, d.y);
                float minD = A.radius + B.radius;
                if (dist < minD && dist > 0.f) {
                    sf::Vector2f n = d / dist;
                    sf::Vector2f rel = A.velocity - B.velocity;
                    float vrel = rel.x * n.x + rel.y * n.y;
                    if (vrel < 0.f) {
                        float jimp = -(1 + restitution) * vrel / 2.f;
                        sf::Vector2f P = jimp * n;
                        A.velocity += P;
                        B.velocity -= P;
                        float overlap = minD - dist;
                        A.shape.move(-n * (overlap * 0.5f));
                        B.shape.move(n * (overlap * 0.5f));
                    }
                }
            }
        }

        // Render
        window.clear(sf::Color::Black);
        window.draw(legend);
        window.draw(containerShape);
        for (auto& b : balls)
            window.draw(b.shape);
        if (dragging) {
            sf::Vector2f m = window.mapPixelToCoords(
                sf::Mouse::getPosition(window)
            );
            sf::Vertex line[] = {
                {dragStart, sf::Color::Yellow},
                {m,         sf::Color::Yellow}
            };
            window.draw(line, 2, sf::Lines);
        }
        window.display();
    }
}
