#include "life/Renderer.hpp"

#include <algorithm>
#include <cstdint>

namespace life {
sf::Color Renderer::color(CellType type) {
    switch (type) {
    case CellType::General: return {100, 220, 155};
    case CellType::Movement: return {80, 170, 250};
    case CellType::Sensory: return {240, 190, 80};
    case CellType::Photosynthetic: return {80, 240, 85};
    case CellType::Storage: return {180, 115, 235};
    case CellType::Defensive: return {235, 105, 110};
    }
    return sf::Color::White;
}
void Renderer::draw(sf::RenderTarget& target, const Simulation& simulation, bool showNutrients) const {
    const auto& field = simulation.nutrients();
    const auto extent = field.extent();
    if (showNutrients) {
        const sf::Vector2f tileSize{extent.x / static_cast<float>(field.columns()), extent.y / static_cast<float>(field.rows())};
        sf::RectangleShape tile(tileSize);
        for (std::size_t y = 0; y < field.rows(); ++y) {
            for (std::size_t x = 0; x < field.columns(); ++x) {
                const auto brightness = static_cast<std::uint8_t>(field.values()[y * field.columns() + x] / NutrientField::capacity * 45.f);
                tile.setPosition({static_cast<float>(x) * tileSize.x, static_cast<float>(y) * tileSize.y});
                tile.setFillColor({8, static_cast<std::uint8_t>(15 + brightness), static_cast<std::uint8_t>(22 + brightness / 2)});
                target.draw(tile);
            }
        }
    }
    sf::CircleShape membrane(1.f, 24);
    sf::CircleShape nucleus(2.f, 12);
    nucleus.setOrigin({2.f, 2.f});
    for (const auto& organism : simulation.organisms()) {
        for (const auto& cell : organism.cells()) {
            const auto position = wrap(organism.position() + cell.position(), extent);
            auto fill = color(cell.type());
            const float vitality = 0.3f + 0.7f * cell.health() / 100.f;
            fill.r = static_cast<std::uint8_t>(static_cast<float>(fill.r) * vitality);
            fill.g = static_cast<std::uint8_t>(static_cast<float>(fill.g) * vitality);
            fill.b = static_cast<std::uint8_t>(static_cast<float>(fill.b) * vitality);
            membrane.setRadius(cell.radius());
            membrane.setOrigin({cell.radius(), cell.radius()});
            membrane.setFillColor(fill);
            membrane.setOutlineThickness(cell.type() == CellType::Defensive ? 2.f : 1.f);
            membrane.setOutlineColor(cell.readyToDivide() ? sf::Color::White : sf::Color{35, 60, 70});
            const auto glow = static_cast<std::uint8_t>(std::clamp(cell.energy() / cell.capacity(), 0.f, 1.f) * 255.f);
            nucleus.setFillColor({glow, glow, glow});
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    const sf::Vector2f point{position.x + static_cast<float>(ox) * extent.x, position.y + static_cast<float>(oy) * extent.y};
                    if (point.x < -14.f || point.y < -14.f || point.x > extent.x + 14.f || point.y > extent.y + 14.f) continue;
                    membrane.setPosition(point);
                    nucleus.setPosition(point);
                    target.draw(membrane);
                    target.draw(nucleus);
                }
            }
        }
    }
}
}
