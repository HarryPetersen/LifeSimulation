#pragma once

#include "life/Cell.hpp"
#include "life/NutrientField.hpp"
#include <span>
#include <vector>

namespace life {
class Organism {
public:
    Organism(std::uint64_t id, Vec2 position, std::vector<Cell> cells, std::uint32_t generation = 0);
    void update(float dt, NutrientField& nutrients, float light, std::mt19937& random, std::uint64_t& nextCellId);
    [[nodiscard]] std::optional<Organism> reproduce(std::uint64_t childId, Vec2 extent);
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] Vec2 position() const noexcept { return position_; }
    [[nodiscard]] Vec2 velocity() const noexcept { return velocity_; }
    [[nodiscard]] std::span<const Cell> cells() const noexcept { return cells_; }
    [[nodiscard]] bool alive() const noexcept { return !cells_.empty(); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return generation_; }
    [[nodiscard]] float energy() const noexcept;
    static constexpr std::size_t maxCells = 24;
private:
    void shareEnergy(float dt);
    void grow(std::mt19937& random, std::uint64_t& nextCellId);
    std::uint64_t id_;
    Vec2 position_;
    Vec2 velocity_{};
    Vec2 heading_{1.f, 0.f};
    std::vector<Cell> cells_;
    std::uint32_t generation_{};
    float reproductionCooldown_{20.f};
};
}
