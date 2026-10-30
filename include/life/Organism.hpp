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
    void integrate(float dt, Vec2 extent);
    void removeDead(NutrientField& nutrients);
    [[nodiscard]] std::optional<Organism> reproduce(std::uint64_t childId, Vec2 extent);
    [[nodiscard]] std::optional<Organism> fragment(std::uint64_t childId, Vec2 extent);
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }
    [[nodiscard]] Vec2 position() const noexcept { return position_; }
    [[nodiscard]] Vec2 velocity() const noexcept { return velocity_; }
    [[nodiscard]] Vec2 worldPosition(const Cell& cell) const noexcept { return position_ + rotate(cell.position(), angle_); }
    [[nodiscard]] std::span<const Cell> cells() const noexcept { return cells_; }
    [[nodiscard]] bool alive() const noexcept { return !cells_.empty(); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return generation_; }
    [[nodiscard]] float energy() const noexcept;
    static constexpr std::size_t maxCells = 24;
private:
    friend class InteractionSystem;
    void shareEnergy(float dt);
    void grow(std::mt19937& random, std::uint64_t& nextCellId);
    void recenter();
    [[nodiscard]] float inverseMass() const noexcept;
    [[nodiscard]] float inverseInertia() const noexcept;
    [[nodiscard]] Vec2 pointVelocity(Vec2 arm) const noexcept;
    void impulse(Vec2 value, Vec2 arm);
    std::uint64_t id_;
    std::uint64_t lineage_;
    Vec2 position_;
    Vec2 velocity_{};
    Vec2 heading_{1.f, 0.f};
    Vec2 externalCue_{};
    float angle_{};
    float angularVelocity_{};
    std::vector<Cell> cells_;
    std::uint32_t generation_{};
    float reproductionCooldown_{20.f};
};
}
