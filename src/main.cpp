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
    sf::Vector2f velocity{0.f, 0.f};
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
        std::cout << "Cell created\n";
    }

    sf::Vector2f getPosition() const {
        return position;
    }
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

void render(std::vector<Cell>& cells, sf::RenderWindow& window){
    for(Cell cell: cells){
        window.draw(cell.shape);
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode({500, 500}), "SFML works!");
    std::cout << "Creating Window\n";

    std::vector<Cell> cells;

    initialise(cells);

    while(window.isOpen()){
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

        
        window.clear();
        render(cells, window);
        window.display();
    }

    return 0;
}