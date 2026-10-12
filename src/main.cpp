#include <iostream>
#include <optional>
#include <vector>
#include <cmath>
#include <SFML/Graphics.hpp>

enum class CellType{
    General,
    Movement,
    Sensory,
};

class Cell {
    CellType type{CellType::General};
    sf::Vector2f localPosition{0.f, 0.f};
    double size{};
    double age{};
    double health{};

public:
    sf::CircleShape shape;

    Cell(CellType cellType, sf::Vector2f pos) : type(cellType), localPosition(pos) {
        shape.setRadius(10.f);
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(localPosition);
        std::cout << "Cell created\n";
    }

    sf::Vector2f getPosition() const {
        return localPosition;
    }

private:
    
};

class Organism{
private:
    sf::Vector2f position{0.f, 0.f};
    sf::Vector2f velocity{30.0f, 10.0f};
    double energy{};
    double drag{0.8};
    std::vector<Cell> cells;

public:
    Organism() {
        cells.emplace_back(CellType::General,  sf::Vector2f{0.f, 0.f});
        cells.emplace_back(CellType::Movement, sf::Vector2f{20.f, 0.f});
        cells.emplace_back(CellType::Sensory,  sf::Vector2f{0.f, 20.f});
    }

    void update(float dt){
        position += velocity * dt;
        float dragFactor = std::pow(drag, dt);

        velocity *= dragFactor;
    }

    void render(sf::RenderWindow& window) const {
        sf::Transform transform;
        transform.translate(position);

        sf::RenderStates states;
        states.transform = transform;

        for (const Cell& cell : cells) {
            window.draw(cell.shape, states);
        }
    }
};

Organism createOrganism(){
    Organism organism;
    return organism;
}

void initialise(std::vector<Organism>& organisms){
    std::cout << "Creating initial organism\n";
    Organism organism = createOrganism();
    organisms.push_back(organism);
}

void update(std::vector<Organism>& organisms, float dt){
    for(Organism& organism: organisms){
        organism.update(dt);
    }
}

void render(const std::vector<Organism>& organisms, sf::RenderWindow& window){
    for(const Organism& organism: organisms){
        organism.render(window);
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode({500, 500}), "Simulation");
    std::cout << "Creating Window\n";
    sf::Clock clock;
    std::vector<Organism> organisms;

    initialise(organisms);

    while(window.isOpen()){
        float dt = clock.restart().asSeconds();
        while(const std::optional event = window.pollEvent()){
            if(event->is<sf::Event::Closed>()) window.close();

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::View view(sf::FloatRect(
                    {0.f, 0.f},
                    {static_cast<float>(resized->size.x),
                     static_cast<float>(resized->size.y)}
                ));

                window.setView(view);
            }
        }

        update(organisms, dt);
        window.clear();
        render(organisms, window);
        window.display();
    }

    return 0;
}