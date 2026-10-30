#include "life/Organism.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace life {
Organism::Organism(std::uint64_t id, Vec2 position, std::vector<Cell> cells, std::uint32_t generation)
    : id_(id), position_(position), cells_(std::move(cells)), generation_(generation) {
    if (!finite(position) || cells_.size() > maxCells) throw std::invalid_argument("Invalid organism");
    for (std::size_t i = 0; i < cells_.size(); ++i)
        for (std::size_t j = i + 1; j < cells_.size(); ++j)
            if (cells_[i].id() == cells_[j].id()) throw std::invalid_argument("Duplicate cell identity");
    cells_.reserve(maxCells);
}
float Organism::energy() const noexcept {
    float total = 0.f;
    for (const auto& cell : cells_) total += cell.energy();
    return total;
}
void Organism::shareEnergy(float dt) {
    for (std::size_t i = 0; i < cells_.size(); ++i) {
        for (std::size_t j = i + 1; j < cells_.size(); ++j) {
            auto& first = cells_[i];
            auto& second = cells_[j];
            if (!first.alive() || !second.alive() || (first.position() - second.position()).length() > 32.f) continue;
            const float difference = first.energy() / first.capacity() - second.energy() / second.capacity();
            auto& donor = difference > 0.f ? first : second;
            auto& receiver = difference > 0.f ? second : first;
            const float amount = std::min({std::abs(difference) * 15.f * dt, donor.energy(), receiver.capacity() - receiver.energy()});
            receiver.receive(donor.withdraw(amount));
        }
    }
}
void Organism::grow(std::mt19937& random, std::uint64_t& nextCellId) {
    const auto existing = cells_.size();
    for (std::size_t i = 0; i < existing && cells_.size() < maxCells; ++i) {
        if (!cells_[i].readyToDivide()) continue;
        const float start = std::uniform_real_distribution<float>(0.f, 2.f * std::numbers::pi_v<float>)(random);
        for (int attempt = 0; attempt < 12; ++attempt) {
            const float angle = start + static_cast<float>(attempt) * std::numbers::pi_v<float> / 6.f;
            const Vec2 offset{26.f * std::cos(angle), 26.f * std::sin(angle)};
            const Vec2 candidate = cells_[i].position() + offset;
            if (candidate.length() > 110.f) continue;
            const bool occupied = std::ranges::any_of(cells_, [&](const Cell& other) {
                return (other.position() - candidate).length() < 25.f;
            });
            if (occupied) continue;
            if (auto child = cells_[i].divide(nextCellId, offset, random)) {
                ++nextCellId;
                cells_.push_back(std::move(*child));
            }
            break;
        }
    }
}
void Organism::update(float dt, NutrientField& nutrients, float light, std::mt19937& random, std::uint64_t& nextCellId) {
    if (!std::isfinite(dt) || dt < 0.f || dt > 0.1f || !std::isfinite(light) || light < 0.f || light > 1.f)
        throw std::invalid_argument("Invalid organism step");
    if (dt == 0.f || !alive()) return;
    reproductionCooldown_ = std::max(0.f, reproductionCooldown_ - dt);
    Vec2 sensed{};
    for (auto& cell : cells_) {
        cell.update(dt, position_, nutrients, light);
        if (cell.alive() && cell.type() == CellType::Sensory)
            sensed += nutrients.gradient(position_ + cell.position(), cell.genome().sensing);
    }
    if (sensed.length() > 0.01f) heading_ = (heading_ + sensed * (dt * 3.f)).normalized();
    shareEnergy(dt);
    float thrust = 0.f;
    for (auto& cell : cells_) {
        if (cell.alive() && cell.type() == CellType::Movement) {
            const float demand = 2.f * dt;
            thrust += cell.genome().thrust * cell.withdraw(demand) / demand * (cell.health() / 100.f);
        }
        if (!cell.alive()) nutrients.deposit(position_ + cell.position(), cell.energy() * 0.5f + 8.f);
    }
    std::erase_if(cells_, [](const Cell& cell) { return !cell.alive(); });
    if (!alive()) return;
    velocity_ += heading_ * (thrust / static_cast<float>(cells_.size()) * dt);
    velocity_ = velocity_ * std::exp(-0.8f * dt);
    position_ = wrap(position_ + velocity_ * dt, nutrients.extent());
    grow(random, nextCellId);
}
std::optional<Organism> Organism::reproduce(std::uint64_t childId, Vec2 extent) {
    if (!finite(extent) || extent.x <= 0.f || extent.y <= 0.f) throw std::invalid_argument("Invalid world extent");
    if (cells_.size() < 12 || reproductionCooldown_ > 0.f || energy() < static_cast<float>(cells_.size()) * 30.f)
        return std::nullopt;
    const auto split = cells_.begin() + static_cast<std::ptrdiff_t>(cells_.size() / 2);
    std::vector<Cell> offspring(split, cells_.end());
    cells_.erase(split, cells_.end());
    Organism child(childId, wrap(position_ + Vec2{45.f, 45.f}, extent), std::move(offspring), generation_ + 1);
    child.velocity_ = velocity_ * -1.f;
    child.heading_ = heading_ * -1.f;
    reproductionCooldown_ = 20.f;
    return child;
}
}
