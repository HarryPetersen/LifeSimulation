#include <iostream>
#include <optional>
#include <vector>
#include <SFML/Graphics.hpp>

enum class CellType{
    General,
    Movement,
    Sensory,
};

class Cell {
    sf::Vector2f position{0.f, 0.f};
    sf::Vector2f velocity{10.0f, 10.0f};
    double energy{};
    double size{};
    double age{};
    CellType type{CellType::General};
    double health{};

public:
    sf::CircleShape shape;

    Cell() {
        shape.setRadius(10.f);
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(position);
        std::cout << "Cell created\n";
    }

    sf::Vector2f getPosition() const {
        return position;
    }

    void update(float dt){
        position += velocity * dt;
        shape.setPosition(position);
    }

private:
    
};

Cell createCell(){
    Cell cell;
    return cell;
}

void initialise(std::vector<Cell>& cells){
    std::cout << "Creating initial objects\n";
    Cell cell = createCell();
    cells.push_back(cell);
}

void update(std::vector<Cell>& cells, float dt){
    for(Cell& cell: cells){
        cell.update(dt);
    }
}

void render(const std::vector<Cell>& cells, sf::RenderWindow& window){
    for(const Cell& cell: cells){
        window.draw(cell.shape);
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode({500, 500}), "Simulation");
    std::cout << "Creating Window\n";
    sf::Clock clock;
    std::vector<Cell> cells;

    initialise(cells);

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

        update(cells, dt);
        window.clear();
        render(cells, window);
        window.display();
    }

    return 0;
}