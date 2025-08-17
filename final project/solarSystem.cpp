
#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <map>
#include <iostream>
#include <cctype>

constexpr float PI = 3.14159265358979323846f;
constexpr float TIME_SCALE = 9999999.f;   // Speed time up for visible orbits
constexpr float AU = 150.f;


struct Star {
    sf::CircleShape shape;
    float twinkleSpeed;
    float twinklePhase;
};

std::vector<Star> generateStars(int count, int width, int height) {
    std::vector<Star> stars;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> distX(0, width);
    std::uniform_int_distribution<int> distY(0, height);
    std::uniform_int_distribution<int> distSize(1, 3);
    std::uniform_real_distribution<float> distSpeed(0.5f, 2.f);
    std::uniform_real_distribution<float> distPhase(0.f, 6.28f);

    for (int i = 0; i < count; ++i) {
        float size = distSize(rng);
        sf::CircleShape star(size);
        star.setOrigin(size, size);
        star.setPosition(distX(rng), distY(rng));
        star.setFillColor(sf::Color::White);

        stars.push_back({ star, distSpeed(rng), distPhase(rng) });
    }
    return stars;
}

struct Planet {
    std::string name;
    float orbitRadius;        // in pixels (scaled)
    float orbitPeriod;        // days
    float radius;             // pixels (used for scaling texture)
    sf::Color baseColor;      // fallback color, not used here since we use textures
    float currentOrbitAngle;  // radians

    std::vector<sf::Vector2f> trail; // past positions for orbit trail

    Planet(const std::string& n, float orbitR, float orbitP, float r, sf::Color c)
        : name(n), orbitRadius(orbitR), orbitPeriod(orbitP), radius(r), baseColor(c), currentOrbitAngle(0)
    {
    }

    void update(float elapsedSeconds) {
        float orbitAngularSpeed = 2 * PI / (orbitPeriod * 86400.f / TIME_SCALE);       // rad/s
        currentOrbitAngle += orbitAngularSpeed * elapsedSeconds;

        if (currentOrbitAngle > 2 * PI) currentOrbitAngle -= 2 * PI;

        if (trail.size() > 80) trail.erase(trail.begin());
    }

    sf::Vector2f getPosition(float cx, float cy) const {
        return sf::Vector2f(
            cx + orbitRadius * std::cos(currentOrbitAngle),
            cy + orbitRadius * std::sin(currentOrbitAngle)
        );
    }

    void addTrailPoint(sf::Vector2f pos) {
        trail.push_back(pos);
    }
};


void runOrbitSimulation() {
    sf::RenderWindow window(sf::VideoMode(1280, 900), "Solar System with Revolution Only", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Vector2f center(window.getSize().x / 2.f, window.getSize().y / 2.f);

    auto stars = generateStars(400, window.getSize().x, window.getSize().y);

    std::vector<Planet> planets = {
        {"Mercury", 0.39f * AU, 88.f, 8.f, sf::Color(169, 169, 169)},
        {"Venus", 0.72f * AU, 224.7f, 14.f, sf::Color(218, 165, 32)},
        {"Earth", 1.f * AU, 365.25f, 16.f, sf::Color(70, 130, 180)},
        {"Mars", 1.52f * AU, 687.f, 12.f, sf::Color(178, 34, 34)},
        {"Jupiter", 5.2f * AU, 4331.f, 30.f, sf::Color(205, 133, 63)},
        {"Saturn", 9.58f * AU, 10747.f, 26.f, sf::Color(210, 180, 140)},
        {"Uranus", 19.22f * AU, 30589.f, 22.f, sf::Color(72, 209, 204)},
        {"Neptune", 30.05f * AU, 59800.f, 22.f, sf::Color(25, 25, 112)}
    };

    sf::Font font;
    if (!font.loadFromFile("OpenSans-Regular.ttf")) {
        std::cerr << "Failed to load font OpenSans-Regular.ttf\n";
    }

    std::map<std::string, sf::Texture> textures;
    std::vector<std::string> textureFiles = {
        "sun.jpg", "mercury.jpg", "venus.jpg", "earth.jpg", "mars.jpg",
        "jupiter.jpg", "saturn.jpg", "uranus.jpg", "neptune.jpg"
    };

    for (const auto& file : textureFiles) {
        sf::Texture tex;
        if (!tex.loadFromFile(file)) {
            std::cerr << "Failed to load texture: " << file << std::endl;
        }
        else {
            textures[file] = std::move(tex);
        }
    }

    // Setup sun sprite
    sf::Sprite sunSprite;
    if (textures.count("sun.jpg")) {
        sunSprite.setTexture(textures["sun.jpg"]);
        sf::Vector2u size = textures["sun.jpg"].getSize();
        sunSprite.setOrigin(size.x / 2.f, size.y / 2.f);
        sunSprite.setPosition(center);
        float scale = (60.f * 2.f) / size.x;  // sun radius 60, so diameter *2
        sunSprite.setScale(scale, scale);
    }

    // Setup planet sprites
    std::vector<sf::Sprite> planetSprites;
    for (const auto& p : planets) {
        std::string filename = p.name;
        for (auto& c : filename) c = static_cast<char>(std::tolower(c));
        filename += ".jpg";

        sf::Sprite sprite;
        if (textures.count(filename)) {
            sprite.setTexture(textures[filename]);
            sf::Vector2u size = textures[filename].getSize();
            sprite.setOrigin(size.x / 2.f, size.y / 2.f);
            float scale = (p.radius * 2.f) / size.x;
            sprite.setScale(scale, scale);
        }
        planetSprites.push_back(sprite);
    }

    sf::Text infoText;
    infoText.setFont(font);
    infoText.setCharacterSize(16);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(10.f, 10.f);

    sf::Clock clock;

    float zoom = 1.f;
    sf::Vector2f viewCenter = center;
    bool dragging = false;
    sf::Vector2i lastMousePos;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (ev.type == sf::Event::KeyPressed) {
                if (ev.key.code == sf::Keyboard::Escape) window.close();
            }

            if (ev.type == sf::Event::MouseWheelScrolled) {
                if (ev.mouseWheelScroll.delta > 0)
                    zoom *= 1.1f;
                else
                    zoom /= 1.1f;
                if (zoom < 0.1f) zoom = 0.1f;
                if (zoom > 10.f) zoom = 10.f;
            }

            if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Middle) {
                dragging = true;
                lastMousePos = sf::Mouse::getPosition(window);
            }
            if (ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Middle) {
                dragging = false;
            }
            if (ev.type == sf::Event::MouseMoved && dragging) {
                sf::Vector2i newPos = sf::Mouse::getPosition(window);
                sf::Vector2f delta = window.mapPixelToCoords(lastMousePos) - window.mapPixelToCoords(newPos);
                viewCenter += delta;
                lastMousePos = newPos;
            }

        }

        float dt = clock.restart().asSeconds();

        // Update planets
        for (auto& p : planets) {
            p.update(dt);
            p.addTrailPoint(p.getPosition(center.x, center.y)); // center of sun
        }



        sf::View view(center, sf::Vector2f(window.getSize().x, window.getSize().y));
        view.setCenter(viewCenter);
        view.setSize(window.getSize().x / zoom, window.getSize().y / zoom);
        window.setView(view);

        window.clear(sf::Color(5, 5, 15));

        for (auto& star : stars) {
            float alpha = (std::sin(star.twinkleSpeed * clock.getElapsedTime().asSeconds() + star.twinklePhase) * 0.5f + 0.5f) * 255.f;
            sf::Color c = star.shape.getFillColor();
            c.a = static_cast<sf::Uint8>(alpha);
            star.shape.setFillColor(c);
            window.draw(star.shape);
        }

        // Draw sun
        if (sunSprite.getTexture() != nullptr) {
            window.draw(sunSprite);
        }
        else {
            // fallback sun as circle
            sf::CircleShape sunFallback(60.f);
            sunFallback.setOrigin(60.f, 60.f);
            sunFallback.setPosition(viewCenter);
            sunFallback.setFillColor(sf::Color(255, 255, 100));
            window.draw(sunFallback);
        }

        // Draw trails and planets
        for (size_t i = 0; i < planets.size(); ++i) {
            const auto& p = planets[i];

            // Draw trail
            if (p.trail.size() > 1) {
                sf::VertexArray trail(sf::LineStrip, p.trail.size());
                for (size_t j = 0; j < p.trail.size(); ++j) {
                    trail[j].position = p.trail[j];
                    sf::Color c = p.baseColor;
                    c.a = static_cast<sf::Uint8>(255 * j / p.trail.size());
                    trail[j].color = c;
                }
                window.draw(trail);
            }

            // Draw planet sprite at current position
            sf::Vector2f pos = p.getPosition(viewCenter.x, viewCenter.y);
            planetSprites[i].setPosition(pos);
            window.draw(planetSprites[i]);
        }

        // Reset view to default for UI
        window.setView(window.getDefaultView());


        infoText.setString(
            "Middle mouse drag: Pan view\n"
            "Mouse wheel: Zoom\n"
        );
        window.draw(infoText);

        window.display();
    }
}
