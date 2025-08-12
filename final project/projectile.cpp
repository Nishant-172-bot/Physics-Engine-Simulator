#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <vector>
#include <sstream>

static constexpr float PI = 3.14159265f;
static constexpr float PROJECTILE_GRAVITY = 500.f;

void runProjectileSimulation() {
    const sf::Vector2u windowSize{ 800, 600 };

    sf::RenderWindow window(
        sf::VideoMode(windowSize.x, windowSize.y),
        "Projectile Motion"
    );
    window.setFramerateLimit(60);

    // Ground
    constexpr float groundH = 4.f;
    sf::RectangleShape ground({ float(windowSize.x), groundH });
    ground.setPosition(0.f, windowSize.y - groundH);
    ground.setFillColor({ 50, 200, 50 });

    // Cannon
    const sf::Vector2f cannonPos{ 50.f, windowSize.y - groundH - 10.f };
    sf::RectangleShape cannon({ 50.f, 20.f });
    cannon.setOrigin(0.f, 10.f);
    cannon.setPosition(cannonPos);
    cannon.setFillColor(sf::Color::Blue);

    // Ball
    constexpr float ballR = 8.f;
    sf::CircleShape ball(ballR);
    ball.setOrigin(ballR, ballR);
    ball.setFillColor(sf::Color::Red);
    ball.setPosition(cannonPos);

    // State flags
    bool aiming = false, launched = false, peakPause = false, landed = false;

    // Physics variables
    sf::Vector2f velocity{ 0.f, 0.f };
    float prevVy = 0.f;
    float maxHeight = 0.f, range = 0.f, projAngle = 0.f, maxVel = 0.f;

    // Trajectory preview
    std::vector<sf::CircleShape> traj;

    // Clocks
    sf::Clock clock, pauseClock, resultClock;
    const float peakPauseDur = 2.f;

    // Font & result text
    sf::Font font;
    font.loadFromFile("OpenSans-Regular.ttf");
    sf::Text results;
    results.setFont(font);
    results.setCharacterSize(18);
    results.setFillColor(sf::Color::White);
    results.setPosition(200.f, 50.f);

    while (window.isOpen()) {
        // Event handling
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            // Begin aiming
            if (e.type == sf::Event::MouseButtonPressed &&
                e.mouseButton.button == sf::Mouse::Left &&
                !launched)
            {
                aiming = true;
                traj.clear();
                ball.setPosition(cannonPos);
                results.setString("");
                landed = launched = false;
            }

            // Launch
            if (e.type == sf::Event::MouseButtonReleased &&
                e.mouseButton.button == sf::Mouse::Left &&
                aiming)
            {
                aiming = false;
                launched = true;

                sf::Vector2f mp = window.mapPixelToCoords(
                    { e.mouseButton.x, e.mouseButton.y }
                );
                sf::Vector2f dir = mp - cannonPos;
                float len = std::hypot(dir.x, dir.y);
                if (len > 0.f) {
                    dir /= len;
                    float speed = std::min(len * 4.f, 1000.f);
                    velocity = dir * speed;
                    maxVel = speed;
                    projAngle = std::atan2(-dir.y, dir.x) * 180.f / PI;
                    ball.setPosition(cannonPos + dir * 50.f);
                }
            }
        }

        float dt = clock.restart().asSeconds();

        // Draw trajectory while aiming
        if (aiming) {
            sf::Vector2f mp = window.mapPixelToCoords(
                sf::Mouse::getPosition(window)
            );
            sf::Vector2f dir = mp - cannonPos;
            float len = std::hypot(dir.x, dir.y);
            if (len > 0.f) {
                dir /= len;
                sf::Vector2f v0 = dir * std::min(len * 4.f, 1000.f);
                traj.clear();
                for (float t = 0.f; t < 2.f; t += 0.1f) {
                    sf::Vector2f p = cannonPos + dir * 50.f
                        + v0 * t
                        + sf::Vector2f(0.f, 0.5f * PROJECTILE_GRAVITY * t * t);
                    if (p.y > windowSize.y) break;
                    sf::CircleShape dot(2.f);
                    dot.setOrigin(2.f, 2.f);
                    dot.setPosition(p);
                    dot.setFillColor(sf::Color::White);
                    traj.push_back(dot);
                }
                cannon.setRotation(std::atan2(dir.y, dir.x) * 180.f / PI);
            }
        }

        // Update physics
        if (launched && !peakPause && !landed) {
            velocity.y += PROJECTILE_GRAVITY * dt;
            ball.move(velocity * dt);

            // Detect peak
            if (velocity.y >= 0.f && prevVy < 0.f) {
                peakPause = true;
                pauseClock.restart();
                maxHeight = cannonPos.y - ball.getPosition().y;
                velocity.y = 0.f;
            }
            prevVy = velocity.y;
        }
        else if (peakPause) {
            if (pauseClock.getElapsedTime().asSeconds() >= peakPauseDur)
                peakPause = false;
        }

        // Detect landing
        if (launched && !landed && !peakPause) {
            if (ball.getPosition().y >= windowSize.y - ballR) {
                ball.setPosition(ball.getPosition().x, windowSize.y - ballR);
                landed = true;
                range = ball.getPosition().x - cannonPos.x;

                std::ostringstream ss;
                ss << "Range: " << range << " px\n"
                    << "Max Height: " << maxHeight << " px\n"
                    << "Angle: " << projAngle << " deg\n"
                    << "Max Velocity: " << maxVel << " px/s";
                results.setString(ss.str());
                resultClock.restart();
            }
        }

        // Hide results after 2s and allow rethrow
        if (landed &&
            resultClock.getElapsedTime().asSeconds() >= 4.f)
        {
            landed = launched = false;
            results.setString("");
            ball.setPosition(cannonPos);
        }

        // Render
        window.clear(sf::Color::Black);
        window.draw(ground);
        window.draw(cannon);
        for (auto& d : traj) window.draw(d);
        window.draw(ball);
        if (landed) window.draw(results);
        window.display();
    }
}
