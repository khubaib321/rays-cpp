#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <numeric>
#include <iostream>

// Constants
const float PI = static_cast<float>(M_PI);
const float HALF_PI = PI / 2.0f;
const float SPEED = 100.0f;
const int FPS_TARGET = 144;
const int FPS_SAMPLES = 144;
float SPEED_MOV = SPEED;
float SPEED_ROT = SPEED;
float DELTA_TIME = 0.0f;
const sf::Color COLOR_BLACK = sf::Color::Black;
const sf::Color COLOR_WHITE = sf::Color::White;
const int CANVAS_WIDTH = 1600;
const int CANVAS_HEIGHT = 900;
const int TOTAL_PIXELS = CANVAS_WIDTH * CANVAS_HEIGHT;

// Directions Enum
enum class Directions {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Drawable abstract class
class Drawable {
public:
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual ~Drawable() {}
};

// Point class
class Point {
public:
    float x;
    float y;

    Point(float x_init = 0.0f, float y_init = 0.0f) : x(x_init), y(y_init) {}

    bool intersects_line(const class Wall& line) const;
};

// Forward declaration
class Wall;

// BoundaryWall class
class BoundaryWall : public Drawable {
public:
    Point start;
    Point end;

    BoundaryWall(const Point& start_point, const Point& end_point)
        : start(start_point), end(end_point) {}

    Point center() const {
        return Point((start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f);
    }

    void draw(sf::RenderWindow& window) override {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(start.x, start.y), COLOR_WHITE),
            sf::Vertex(sf::Vector2f(end.x, end.y), COLOR_WHITE)
        };
        window.draw(line, 2, sf::Lines);
    }

    float get_angle(Directions direction) const {
        float dx_line, dy_line;
        if (direction == Directions::LEFT) {
            dx_line = start.x - end.x;
            dy_line = start.y - end.y;
        } else if (direction == Directions::RIGHT) {
            dx_line = end.x - start.x;
            dy_line = end.y - start.y;
        } else {
            throw std::runtime_error("Direction not implemented");
        }
        return std::atan2(dy_line, dx_line);
    }

    bool intersects_line(const Wall& line) const;
};

// Wall class
class Wall : public BoundaryWall {
public:
    using BoundaryWall::BoundaryWall;

    void draw(sf::RenderWindow& window) override;

    void rotate(float angle_degrees);

    bool intersects_line(const BoundaryWall& line) const {
        // Implementation of line intersection
        const Point& A = start;
        const Point& B = end;
        const Point& C = line.start;
        const Point& D = line.end;

        auto ccw = [](const Point& A, const Point& B, const Point& C) {
            return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
        };

        return ccw(A, C, D) != ccw(B, C, D) && ccw(A, B, C) != ccw(A, B, D);
    }
};

bool Point::intersects_line(const Wall& line) const {
    float dist_from_end = std::hypot(x - line.end.x, y - line.end.y);
    float dist_from_start = std::hypot(x - line.start.x, y - line.start.y);
    float line_length = std::hypot(line.start.x - line.end.x, line.start.y - line.end.y);

    return std::abs(dist_from_end + dist_from_start - line_length) < 0.01f;
}

void Wall::draw(sf::RenderWindow& window) {
    BoundaryWall::draw(window);
}

void Wall::rotate(float angle_degrees) {
    Point c = center();
    float angle_radians = angle_degrees * (PI / 180.0f);

    // Translate the line to origin
    float translated_end_x = end.x - c.x;
    float translated_end_y = end.y - c.y;
    float translated_start_x = start.x - c.x;
    float translated_start_y = start.y - c.y;

    // Rotate the line
    float cos_theta = std::cos(angle_radians);
    float sin_theta = std::sin(angle_radians);

    float rotated_start_x = translated_start_x * cos_theta - translated_start_y * sin_theta;
    float rotated_start_y = translated_start_x * sin_theta + translated_start_y * cos_theta;
    float rotated_end_x = translated_end_x * cos_theta - translated_end_y * sin_theta;
    float rotated_end_y = translated_end_x * sin_theta + translated_end_y * cos_theta;

    // Translate line back to original coordinates
    start.x = rotated_start_x + c.x;
    start.y = rotated_start_y + c.y;
    end.x = rotated_end_x + c.x;
    end.y = rotated_end_y + c.y;
}

// Ray class
class Ray : public Drawable {
public:
    sf::Color color;
    Point source;
    float length;
    Point end_point;
    sf::Vector2f direction;

    Ray(const Point& source_point, float angle, float length_value, const sf::Color& ray_color)
        : color(ray_color), source(source_point), length(length_value) {
        direction = sf::Vector2f(std::cos(angle), std::sin(angle)) * length;
    }

    void draw(sf::RenderWindow& window) override {
        set_end_point();
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(source.x, source.y), color),
            sf::Vertex(sf::Vector2f(end_point.x, end_point.y), color)
        };
        window.draw(line, 2, sf::Lines);
    }

    void set_end_point();

    Point intersects_line(const BoundaryWall& line) const;
};

// Globals
std::vector<std::shared_ptr<BoundaryWall> > WALLS;

void Ray::set_end_point() {
    float nearest_wall_distance = length;
    bool found_intersection = false;

    for (std::size_t i = 0; i < WALLS.size(); ++i) {
        Point intersects_at = intersects_line(*WALLS[i]);
        if (intersects_at.x != -1) {
            float distance_to_wall = std::hypot(source.x - intersects_at.x, source.y - intersects_at.y);
            if (distance_to_wall < nearest_wall_distance) {
                nearest_wall_distance = distance_to_wall;
                end_point = intersects_at;
                found_intersection = true;
            }
        }
    }

    if (!found_intersection) {
        end_point = Point(source.x + direction.x, source.y + direction.y);
    }
}

Point Ray::intersects_line(const BoundaryWall& line) const {
    float x1 = source.x;
    float y1 = source.y;
    float x2 = source.x + direction.x;
    float y2 = source.y + direction.y;

    float x3 = line.start.x;
    float y3 = line.start.y;
    float x4 = line.end.x;
    float y4 = line.end.y;

    // Denominator
    float denominator = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);

    // Check for parallel lines
    if (denominator == 0) {
        return Point(-1, -1); // No intersection
    }

    float ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
    float ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;

    // Check if intersection is within the line segments
    if (ua < 0 || ua > 1 || ub < 0 || ub > 1) {
        return Point(-1, -1); // No intersection
    }

    float x = x1 + ua * (x2 - x1);
    float y = y1 + ua * (y2 - y1);

    return Point(x, y);
}

// LightSource class
class LightSource : public Drawable {
public:
    sf::Color color;
    int ray_density;
    Point pos;
    float radius;
    float mouse_x_old;
    float mouse_y_old;

    LightSource(const sf::Color& light_color, int density = 1)
        : color(light_color), ray_density(density), pos(0, 0), radius(5.0f), mouse_x_old(0.0f), mouse_y_old(0.0f) {}

    void draw(sf::RenderWindow& window);

    void draw_rays(sf::RenderWindow& window);

    Point intersects_any_line(const Point& new_pos, Directions direction);
};

void LightSource::draw(sf::RenderWindow& window) {
    // Handle movement
    sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
    // Prioritize mouse movement over WASD
    if (mouse_pos.x != static_cast<int>(mouse_x_old) || mouse_pos.y != static_cast<int>(mouse_y_old)) {
        pos.x = static_cast<float>(mouse_pos.x);
        pos.y = static_cast<float>(mouse_pos.y);
        mouse_x_old = static_cast<float>(mouse_pos.x);
        mouse_y_old = static_cast<float>(mouse_pos.y);
    } else {
        // WASD movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            pos.y = std::max(0.0f, pos.y - SPEED_MOV * DELTA_TIME);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            pos.y = std::min(static_cast<float>(CANVAS_HEIGHT), pos.y + SPEED_MOV * DELTA_TIME);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            float new_x_lat = std::max(0.0f, pos.x - SPEED_MOV * DELTA_TIME);
            pos = intersects_any_line(Point(new_x_lat, pos.y), Directions::LEFT);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            float new_x_lat = std::min(static_cast<float>(CANVAS_WIDTH), pos.x + SPEED_MOV * DELTA_TIME);
            pos = intersects_any_line(Point(new_x_lat, pos.y), Directions::RIGHT);
        }
    }

    sf::CircleShape circle(radius);
    circle.setFillColor(color);
    circle.setPosition(pos.x - radius, pos.y - radius);
    window.draw(circle);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        draw_rays(window);
    }
}

void LightSource::draw_rays(sf::RenderWindow& window) {
    sf::VertexArray lines(sf::Lines);

    for (int i = 0; i < ray_density; ++i) {
        float angle = i * 2 * PI / ray_density;
        Ray ray(pos, angle, static_cast<float>(TOTAL_PIXELS), color);
        
        // ray.draw(window);
        // To reduce draw calls (and potential gpu bottleneck), set the ray endpoint explicitly 
        // and batch all lines into a single draw call
        ray.set_end_point(); // Call this explicitly
        
        // Add the vertices to the VertexArray
        lines.append(sf::Vertex(sf::Vector2f(ray.source.x, ray.source.y), ray.color));
        lines.append(sf::Vertex(sf::Vector2f(ray.end_point.x, ray.end_point.y), ray.color));
    }
    
    // Draw all rays in one call
    // Real impact: FPS boosted from ~40 to ~90 with 46080 rays!
    window.draw(lines);
}

Point LightSource::intersects_any_line(const Point& new_pos, Directions direction) {
    Wall temp_line(pos, new_pos);
    for (std::size_t i = 0; i < WALLS.size(); ++i) {
        if (temp_line.intersects_line(*WALLS[i])) {
            float wall_angle = WALLS[i]->get_angle(direction);

            if (wall_angle == PI || wall_angle == -PI || wall_angle == HALF_PI / 2 || wall_angle == -HALF_PI / 2) {
                return pos;
            }

            float move_length = std::hypot(new_pos.x - pos.x, new_pos.y - pos.y);
            sf::Vector2f direction_vector(std::cos(wall_angle), std::sin(wall_angle));
            direction_vector *= move_length;
            return Point(pos.x + direction_vector.x, pos.y + direction_vector.y);
        }
    }
    return new_pos;
}

// Function to show stats
void show_stats(sf::RenderWindow& window, sf::Font& font, float fps) {
    std::string stats = "SPEED_MOV: " + std::to_string(static_cast<int>(SPEED_MOV)) +
                        ", SPEED_ROT: " + std::to_string(static_cast<int>(SPEED_ROT)) +
                        ", FPS: " + std::to_string(static_cast<int>(fps));

    sf::Text text;
    text.setFont(font);
    text.setString(stats);
    text.setCharacterSize(24);
    text.setFillColor(COLOR_WHITE);
    text.setPosition(10.0f, 10.0f);

    window.draw(text);
}

int main() {
    sf::Font font;
    // Initialize SFML
    sf::RenderWindow window(sf::VideoMode(CANVAS_WIDTH, CANVAS_HEIGHT), "Rays");
    // Disable frame rate limiting
    // window.setFramerateLimit(0);
    // window.setFramerateLimit(FPS_TARGET);
    window.setVerticalSyncEnabled(true);
    
    if (!font.loadFromFile("res/Arial.ttf")) {
        std::cerr << "Error loading font.\n";
        return -1;
    }

    // Walls setup
    WALLS.push_back(std::make_shared<Wall>(Point(300, 100), Point(500, 300)));
    WALLS.push_back(std::make_shared<Wall>(Point(200, 600), Point(500, 800)));
    WALLS.push_back(std::make_shared<Wall>(Point(600, 300), Point(600, 500)));
    WALLS.push_back(std::make_shared<Wall>(Point(800, 600), Point(1000, 600)));
    WALLS.push_back(std::make_shared<Wall>(Point(1200, 100), Point(1200, 700)));
    // Scene boundaries
    WALLS.push_back(std::make_shared<BoundaryWall>(Point(0, 0), Point(CANVAS_WIDTH, 0)));
    WALLS.push_back(std::make_shared<BoundaryWall>(Point(0, 0), Point(0, CANVAS_HEIGHT)));
    WALLS.push_back(std::make_shared<BoundaryWall>(Point(CANVAS_WIDTH, 0), Point(CANVAS_WIDTH, CANVAS_HEIGHT)));
    WALLS.push_back(std::make_shared<BoundaryWall>(Point(0, CANVAS_HEIGHT), Point(CANVAS_WIDTH, CANVAS_HEIGHT)));

    // Initialize scene objects
    std::vector<std::shared_ptr<Drawable> > DRAWABLES;
    for (std::size_t i = 0; i < WALLS.size(); ++i) {
        DRAWABLES.push_back(WALLS[i]);
    }

    const float rayDensity = 46080;
    std::shared_ptr<LightSource> light = std::make_shared<LightSource>(sf::Color(253, 184, 19), rayDensity);
    DRAWABLES.push_back(light);

    const int FPS_SAMPLES = 60;
    std::deque<float> fpsSamples;

    bool running = true;
    sf::Clock clock;
    float fps = 0.0f;
    float avgFps = 0.0f;
    float DELTA_TIME = 0.0f;
    // Main loop
    while (running) {
        // Start measuring time
        sf::Time frameStartTime = clock.getElapsedTime();

        // Event Handling
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
            }
        }

        // Speed adjustments
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
            SPEED_MOV += 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen)) {
            SPEED_MOV = std::abs(SPEED_MOV - 1);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            SPEED_ROT = std::abs(SPEED_ROT - 1);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            SPEED_ROT += 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            SPEED_MOV = SPEED_ROT = SPEED;
        }

        // Rotate walls if needed
        for (std::size_t i = 0; i < WALLS.size(); ++i) {
            std::shared_ptr<Wall> wall = std::dynamic_pointer_cast<Wall>(WALLS[i]);
            if (wall) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                    wall->rotate(SPEED_ROT * DELTA_TIME);
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                    wall->rotate(-SPEED_ROT * DELTA_TIME);
                }
            }
        }

        // Clear the window
        window.clear(COLOR_BLACK);

        // Draw all objects
        for (std::size_t i = 0; i < DRAWABLES.size(); ++i) {
            DRAWABLES[i]->draw(window);
        }

        // Update the stats & display
        show_stats(window, font, avgFps);
        window.display();

        // End measuring time
        sf::Time frameEndTime = clock.getElapsedTime();

        // Calculate DELTA_TIME
        DELTA_TIME = (frameEndTime - frameStartTime).asSeconds();

        // Calculate FPS and Average FPS
        fps = 1.0f / DELTA_TIME;
        fpsSamples.push_back(fps);
        if (fpsSamples.size() > FPS_SAMPLES) {
            fpsSamples.pop_front();
        }
        avgFps = std::accumulate(fpsSamples.begin(), fpsSamples.end(), 0.0f) / fpsSamples.size();
    }

    return 0;
}