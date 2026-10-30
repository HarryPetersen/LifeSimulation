#pragma once

#include "life/Simulation.hpp"
#include <SFML/Graphics.hpp>

namespace life {
class Renderer {
public:
    void draw(sf::RenderTarget& target, const Simulation& simulation, bool showNutrients = true) const;
private:
    static sf::Color color(CellType type);
};
}
