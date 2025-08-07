#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>
#include <sstream>
#include <random>

static constexpr float PI = 3.14159265f;
static constexpr float TIME_SCALE = 5.f;
static constexpr float ORBIT_GRAVITY = 0.1f;
static constexpr float PROJECTILE_GRAVITY = 500.f;

// -------------------- Base Simulation --------------------
class Simulation {
public:
    virtual void run() = 0;
    virtual ~Simulation() = default;
};
// -------------------- 1) Planetary Orbit --------------------
class OrbitSimulation : public Simulation {
    const sf::Vector2u windowSize{ 800, 600 };

public:
    void run() override {
        sf::RenderWindow window(
            sf::VideoMode(windowSize.x, windowSize.y),
            "Solar System"
        );
        window.setFramerateLimit(60);

        const sf::Vector2f center(
            windowSize.x * 0.5f,
            windowSize.y * 0.5f
        );
        const float rEarth = 200.f;
        const float rSaturn = 300.f;
        const float sunMass = 10000.f;

        // Sun: serrated-edge using ConvexShape
        const float sunRadius = 60.f;
        const int   spikes = 60;
        sf::ConvexShape sun;
        sun.setPointCount(spikes);
        for (int i = 0; i < spikes; ++i) {
            float angle = 2 * PI * i / spikes;
            float offset = (i % 2) ? 10.f : -3.f;
            float rad = sunRadius + offset;
            sun.setPoint(i, {
                std::cos(angle) * rad,
                std::sin(angle) * rad
                });
        }
        // Center origin based on its local bounds
        {
            auto lb = sun.getLocalBounds();
            sun.setOrigin(lb.left + lb.width * 0.5f,
                lb.top + lb.height * 0.5f);
        }
        sun.setPosition(center);
        sun.setFillColor(sf::Color::Yellow);

        // Orbit paths
        sf::CircleShape pathE(rEarth), pathS(rSaturn);
        pathE.setOrigin(rEarth, rEarth);
        pathS.setOrigin(rSaturn, rSaturn);
        pathE.setPosition(center);
        pathS.setPosition(center);
        pathE.setFillColor(sf::Color::Transparent);
        pathS.setFillColor(sf::Color::Transparent);
        pathE.setOutlineThickness(1.f);
        pathS.setOutlineThickness(1.f);
        pathE.setOutlineColor(sf::Color::White);
        pathS.setOutlineColor(sf::Color::White);

        // Earth: circular with stripes
        const float earthR = 15.f;
        sf::CircleShape earth(earthR, 40);
        earth.setOrigin(earthR, earthR);
        earth.setPosition(center + sf::Vector2f(rEarth, 0.f));
        earth.setFillColor(sf::Color(64, 164, 223));
        std::vector<sf::RectangleShape> earthStripes;
        for (int i = 0; i < 6; ++i) {
            sf::RectangleShape stripe({ 4.f, earthR * 2.f });
            stripe.setOrigin(2.f, earthR);
            stripe.setPosition(earth.getPosition());
            stripe.setFillColor(sf::Color(139, 69, 19));
            stripe.setRotation(i * 60.f);
            earthStripes.push_back(stripe);
        }

        // Saturn: circular + ring
        const float satR = 20.f;
        sf::CircleShape sat(satR, 40);
        sat.setOrigin(satR, satR);
        sat.setPosition(center + sf::Vector2f(rSaturn, 0.f));
        sat.setFillColor(sf::Color(210, 180, 140));
        sf::CircleShape ring(satR * 1.4f, 60);
        ring.setOrigin(satR * 1.4f, satR * 1.4f);
        ring.setPosition(sat.getPosition());
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(180, 180, 180));
        ring.setOutlineThickness(4.f);
        ring.setScale(1.f, 0.5f);

        // Initial orbital velocities
        sf::Vector2f velE(
            0.f,
            std::sqrt(ORBIT_GRAVITY * sunMass / rEarth)
        );
        sf::Vector2f velS(
            0.f,
            std::sqrt(ORBIT_GRAVITY * sunMass / rSaturn)
        );

        sf::Clock clock;
        while (window.isOpen()) {
            sf::Event e;
            while (window.pollEvent(e))
                if (e.type == sf::Event::Closed)
                    window.close();

            float dt = clock.restart().asSeconds() * TIME_SCALE * 3.f;

            auto updateOrbit = [&](sf::CircleShape& planet, sf::Vector2f& vel) {
                sf::Vector2f toSun = center - planet.getPosition();
                float d2 = toSun.x * toSun.x + toSun.y * toSun.y;
                float d = std::sqrt(d2);
                if (d > 1.f) {
                    float a = ORBIT_GRAVITY * sunMass / d2;
                    vel += (toSun / d) * (a * dt);
                }
                planet.move(vel * dt);
                };

            updateOrbit(earth, velE);
            updateOrbit(sat, velS);

            // Keep stripes and ring following planets
            for (auto& s : earthStripes)
                s.setPosition(earth.getPosition());
            ring.setPosition(sat.getPosition());

            // Draw
            window.clear(sf::Color::Black);
            window.draw(pathE);
            window.draw(pathS);
            window.draw(sun);
            for (auto& s : earthStripes)
                window.draw(s);
            window.draw(earth);
            window.draw(ring);
            window.draw(sat);
            window.display();
        }
    }
};




// -------------------- 2) Projectile Motion --------------------
class ProjectileSimulation : public Simulation {
    const sf::Vector2u windowSize{ 800, 600 };

public:
    void run() override {
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
        font.loadFromFile("arial.ttf");
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
};


class CollisionSimulation : public Simulation {
    const sf::Vector2u windowSize{ 800, 600 };
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

public:
    void run() override {
        sf::RenderWindow window(
            sf::VideoMode(windowSize.x, windowSize.y),
            "Container Collision"
        );
        window.setFramerateLimit(60);

        // Legend
        sf::Font font;
        font.loadFromFile("Arial.ttf");
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
};


// -------------------- 4) Viscosity Simulation --------------------
class ViscositySimulation : public Simulation {
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

    sf::RenderWindow window;
    sf::RectangleShape ground;
    std::vector<sf::RectangleShape> containers;
    std::vector<PhysicsCircle> balls;
    std::vector<sf::Text> labels;
    std::vector<float> phases;
    sf::Font font;
    bool isRunning;
    sf::Clock clock;
    static constexpr int COUNT = 5;
    std::vector<float> viscosityValues{ 5.f,8.f,15.f,50.f,30.f };
    std::vector<std::string> names{ "Water","Alcohol","Oil","Honey","Glycerine" };
    std::vector<sf::Color> colors{
        sf::Color(64,164,223,180), sf::Color(194,245,255,180),
        sf::Color(255,222,89,180), sf::Color(204,142,53,200), sf::Color(230,230,255,200)
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

public:
    ViscositySimulation()
        : window(sf::VideoMode::getDesktopMode(), "Viscosity Simulation"),
        isRunning(false)
    {
        window.setFramerateLimit(60);
        float gh = 350.f;
        ground.setSize({ (float)window.getSize().x, gh });
        ground.setPosition(0.f, window.getSize().y - gh);
        ground.setFillColor(sf::Color::Black);

        if (!font.loadFromFile("arial.ttf")) std::cerr << "Font load failed\n";

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
    }

    void run() override {
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
};

int main() {
    std::unique_ptr<Simulation> sim;
    while (true) {
        std::cout << "Select simulation:\n"
            << " 1) Planetary Orbit\n"
            << " 2) Projectile Motion\n"
            << " 3) Collision Simulation\n"
            << " 4) Viscosity Simulation\n"
            << " 5) Quit\n"
            << "Enter choice: ";
        int choice;
        if (!(std::cin >> choice)) break;
        switch (choice) {
        case 1: sim = std::make_unique<OrbitSimulation>();      break;
        case 2: sim = std::make_unique<ProjectileSimulation>(); break;
        case 3: sim = std::make_unique<CollisionSimulation>();  break;
        case 4: sim = std::make_unique<ViscositySimulation>();  break;
        case 5: std::cout << "Exiting...\n"; return 0;
        default:
            std::cout << "Invalid choice, try again.\n";
            continue;
        }
        sim->run();
    }
    return 0;
}

