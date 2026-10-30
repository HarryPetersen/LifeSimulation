#pragma once

#include "life/Genome.hpp"
#include "life/Vector.hpp"
#include <cstdint>
#include <optional>

namespace life {
class NutrientField;
enum class CellPhase : std::uint8_t { Growing, Mature, Dividing, Dead };

class Cell {
public:
    Cell(std::uint64_t id, CellType type, Vec2 localPosition, Genome genome = {});
    void update(float dt, Vec2 worldPosition, NutrientField& nutrients, float light);
    [[nodiscard]] std::optional<Cell> divide(std::uint64_t childId, Vec2 offset, std::mt19937& random);
    void damage(float amount);
    float withdraw(float amount);
    float receive(float amount);
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] CellType type() const noexcept { return type_; }
    [[nodiscard]] CellPhase phase() const noexcept { return phase_; }
    [[nodiscard]] const Genome& genome() const noexcept { return genome_; }
    [[nodiscard]] Vec2 position() const noexcept { return position_; }
    [[nodiscard]] float radius() const noexcept { return genome_.radius * (0.6f + 0.4f * growth_); }
    [[nodiscard]] float energy() const noexcept { return energy_; }
    [[nodiscard]] float capacity() const noexcept { return type_ == CellType::Storage ? 180.f : 100.f; }
    [[nodiscard]] float health() const noexcept { return health_; }
    [[nodiscard]] float distress() const noexcept { return distress_; }
    [[nodiscard]] float age() const noexcept { return age_; }
    [[nodiscard]] bool alive() const noexcept { return phase_ != CellPhase::Dead; }
    [[nodiscard]] bool readyToDivide() const noexcept { return phase_ == CellPhase::Dividing; }
private:
    friend class Organism;
    std::uint64_t id_;
    CellType type_;
    Vec2 position_;
    Genome genome_;
    CellPhase phase_{CellPhase::Growing};
    float age_{};
    float sinceDivision_{};
    float growth_{0.5f};
    float health_{100.f};
    float energy_{50.f};
    float distress_{};
};
}
