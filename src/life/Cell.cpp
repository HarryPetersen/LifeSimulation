#include "life/Cell.hpp"
#include "life/NutrientField.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace life {
namespace {
void requireAmount(float amount) {
    if (!std::isfinite(amount) || amount < 0.f) throw std::invalid_argument("Amount must be finite and non-negative");
}
}
Cell::Cell(std::uint64_t id, CellType type, Vec2 localPosition, Genome genome)
    : id_(id), type_(type), position_(localPosition), genome_(genome) {
    genome_.validate();
    if (!finite(position_) || static_cast<unsigned>(type) > static_cast<unsigned>(CellType::Predator))
        throw std::invalid_argument("Invalid cell position or type");
}
float Cell::withdraw(float amount) {
    requireAmount(amount);
    const float taken = std::min(amount, energy_);
    energy_ -= taken;
    return taken;
}
float Cell::receive(float amount) {
    requireAmount(amount);
    const float accepted = alive() ? std::min(amount, capacity() - energy_) : 0.f;
    energy_ += accepted;
    return accepted;
}
void Cell::damage(float amount) {
    requireAmount(amount);
    if (amount > 0.f && alive()) distress_ = std::min(1.f, distress_ + amount * 0.15f);
    health_ = std::max(0.f, health_ - amount * (type_ == CellType::Defensive ? 0.4f : 1.f));
    if (health_ == 0.f) phase_ = CellPhase::Dead;
}
void Cell::update(float dt, Vec2 worldPosition, NutrientField& nutrients, float light) {
    if (!std::isfinite(dt) || dt < 0.f || dt > 0.1f || !finite(worldPosition) ||
        !std::isfinite(light) || light < 0.f || light > 1.f) throw std::invalid_argument("Invalid cell environment");
    if (!alive() || dt == 0.f) return;
    distress_ = std::max(0.f, distress_ - dt * 0.3f);
    age_ += dt;
    sinceDivision_ += dt;
    const float uptakeScale = type_ == CellType::General ? 1.4f : 0.7f;
    receive(nutrients.consume(worldPosition, std::min(capacity() - energy_, genome_.uptake * uptakeScale * dt)));
    if (type_ == CellType::Photosynthetic) receive(6.f * light * dt);
    const float cost = genome_.metabolism * (type_ == CellType::Sensory ? 1.4f : 1.f) * dt;
    const float paid = withdraw(cost);
    if (paid < cost) damage((cost - paid) * 12.f);
    if (age_ > genome_.lifespan) damage(dt * 8.f);
    if (!alive()) return;
    if (health_ < 100.f && energy_ > 15.f) {
        const float repaired = std::min(100.f - health_, dt * 2.f);
        withdraw(repaired * 0.5f);
        health_ += repaired;
    }
    if (growth_ < 1.f && energy_ > 20.f) {
        const float growth = std::min(1.f - growth_, dt * 0.06f);
        withdraw(growth * 15.f);
        growth_ += growth;
    }
    phase_ = growth_ < 1.f ? CellPhase::Growing : CellPhase::Mature;
    if (growth_ >= 1.f && sinceDivision_ >= genome_.divisionAge && energy_ >= 80.f && health_ >= 70.f)
        phase_ = CellPhase::Dividing;
}
std::optional<Cell> Cell::divide(std::uint64_t childId, Vec2 offset, std::mt19937& random) {
    if (!finite(offset)) throw std::invalid_argument("Invalid division offset");
    if (!readyToDivide() || energy_ < 80.f || !alive()) return std::nullopt;
    CellType childType = type_;
    if (std::bernoulli_distribution(genome_.mutationRate)(random))
        childType = static_cast<CellType>(std::uniform_int_distribution<int>(0, 7)(random));
    Cell child(childId, childType, position_ + offset, genome_.mutated(random));
    energy_ -= 12.f;
    child.energy_ = energy_ * 0.5f;
    energy_ -= child.energy_;
    child.growth_ = 0.5f;
    growth_ = 0.5f;
    sinceDivision_ = 0.f;
    phase_ = CellPhase::Growing;
    return child;
}
}
