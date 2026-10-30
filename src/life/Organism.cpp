#include "life/Organism.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace life {
Organism::Organism(std::uint64_t id, Vec2 position, std::vector<Cell> cells, std::uint32_t generation)
    : id_(id), lineage_(id), position_(position), cells_(std::move(cells)), generation_(generation) {
    if (!finite(position) || cells_.size() > maxCells) throw std::invalid_argument("Invalid organism");
    for (std::size_t i = 0; i < cells_.size(); ++i)
        for (std::size_t j = i + 1; j < cells_.size(); ++j)
            if (cells_[i].id() == cells_[j].id()) throw std::invalid_argument("Duplicate cell identity");
    cells_.reserve(maxCells);
    recenter();
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
    Vec2 sensed = externalCue_;
    externalCue_ = {};
    for (auto& cell : cells_) {
        cell.update(dt, worldPosition(cell), nutrients, light);
        if (cell.alive() && cell.type() == CellType::Sensory)
            sensed += nutrients.gradient(worldPosition(cell), cell.genome().sensing);
    }
    if (sensed.length() > 0.01f) heading_ = (heading_ + sensed * (dt * 3.f)).normalized();
    shareEnergy(dt);
    float thrust = 0.f;
    for (auto& cell : cells_) {
        if (cell.alive() && cell.type() == CellType::Movement) {
            const float demand = 2.f * dt;
            thrust += cell.genome().thrust * cell.withdraw(demand) / demand * (cell.health() / 100.f);
        }
    }
    removeDead(nutrients);
    if (!alive()) return;
    velocity_ += heading_ * (thrust * inverseMass() * dt);
    velocity_ = velocity_ * std::exp(-0.8f * dt);
    grow(random, nextCellId);
    recenter();
    position_ = wrap(position_, nutrients.extent());
}
std::optional<Organism> Organism::reproduce(std::uint64_t childId, Vec2 extent) {
    if (!finite(extent) || extent.x <= 0.f || extent.y <= 0.f) throw std::invalid_argument("Invalid world extent");
    if (cells_.size() < 12 || reproductionCooldown_ > 0.f || energy() < static_cast<float>(cells_.size()) * 30.f)
        return std::nullopt;
    std::ranges::stable_sort(cells_, {}, [](const Cell& cell) { return cell.position().x; });
    const auto split = cells_.begin() + static_cast<std::ptrdiff_t>(cells_.size() / 2);
    std::vector<Cell> offspring(split, cells_.end());
    cells_.erase(split, cells_.end());
    const Vec2 origin = position_;
    for (auto& cell : offspring) cell.position_ = rotate(cell.position_, angle_);
    Organism child(childId, origin, std::move(offspring), generation_ + 1);
    child.lineage_ = lineage_;
    child.velocity_ = velocity_ + perpendicular(child.position_ - origin) * angularVelocity_;
    child.angularVelocity_ = angularVelocity_;
    child.heading_ = heading_;
    recenter();
    child.position_ = wrap(child.position_, extent);
    position_ = wrap(position_, extent);
    reproductionCooldown_ = 20.f;
    return child;
}
std::optional<Organism> Organism::fragment(std::uint64_t childId, Vec2 extent) {
    if (!finite(extent) || extent.x <= 0.f || extent.y <= 0.f) throw std::invalid_argument("Invalid world extent");
    if (cells_.size() < 2) return std::nullopt;
    std::vector<bool> connected(cells_.size(), false);
    std::vector<std::size_t> pending{0};
    connected[0] = true;
    for (std::size_t cursor = 0; cursor < pending.size(); ++cursor) {
        const auto current = pending[cursor];
        for (std::size_t i = 0; i < cells_.size(); ++i) {
            if (!connected[i] && (cells_[i].position() - cells_[current].position()).length() <= 32.f) {
                connected[i] = true;
                pending.push_back(i);
            }
        }
    }
    if (pending.size() == cells_.size()) return std::nullopt;
    std::vector<Cell> retained, detached;
    retained.reserve(maxCells);
    detached.reserve(maxCells);
    for (std::size_t i = 0; i < cells_.size(); ++i) {
        if (connected[i]) retained.push_back(cells_[i]);
        else {
            detached.push_back(cells_[i]);
            detached.back().position_ = rotate(detached.back().position_, angle_);
        }
    }
    const Vec2 origin = position_;
    cells_ = std::move(retained);
    Organism child(childId, origin, std::move(detached), generation_);
    child.lineage_ = lineage_;
    child.heading_ = heading_;
    child.velocity_ = velocity_ + perpendicular(child.position_ - origin) * angularVelocity_;
    child.angularVelocity_ = angularVelocity_;
    recenter();
    position_ = wrap(position_, extent);
    child.position_ = wrap(child.position_, extent);
    return child;
}
float Organism::inverseMass() const noexcept {
    float mass = 0.f;
    for (const auto& cell : cells_) mass += cell.radius() * cell.radius() / 49.f;
    return mass > 0.f ? 1.f / mass : 0.f;
}
float Organism::inverseInertia() const noexcept {
    float inertia = 0.f;
    for (const auto& cell : cells_) {
        const float radiusSquared = cell.radius() * cell.radius();
        inertia += radiusSquared / 49.f * (dot(cell.position(), cell.position()) + radiusSquared * 0.5f);
    }
    return inertia > 0.f ? 1.f / inertia : 0.f;
}
Vec2 Organism::pointVelocity(Vec2 arm) const noexcept {
    return velocity_ + perpendicular(arm) * angularVelocity_;
}
void Organism::impulse(Vec2 value, Vec2 arm) {
    velocity_ += value * inverseMass();
    angularVelocity_ += cross(arm, value) * inverseInertia();
    const float speed = velocity_.length();
    if (speed > 80.f) velocity_ = velocity_ * (80.f / speed);
    angularVelocity_ = std::clamp(angularVelocity_, -1.f, 1.f);
}
void Organism::integrate(float dt, Vec2 extent) {
    if (!std::isfinite(dt) || dt < 0.f || dt > 0.1f || !finite(extent) || extent.x <= 0.f || extent.y <= 0.f)
        throw std::invalid_argument("Invalid integration step");
    impulse({}, {});
    position_ = wrap(position_ + velocity_ * dt, extent);
    angle_ = std::remainder(angle_ + angularVelocity_ * dt, 2.f * std::numbers::pi_v<float>);
    angularVelocity_ *= std::exp(-1.2f * dt);
}
void Organism::recenter() {
    if (cells_.empty()) return;
    Vec2 center{};
    float mass = 0.f;
    for (const auto& cell : cells_) {
        const float weight = cell.radius() * cell.radius();
        center += cell.position() * weight;
        mass += weight;
    }
    center = center * (1.f / mass);
    const Vec2 shift = rotate(center, angle_);
    position_ += shift;
    velocity_ += perpendicular(shift) * angularVelocity_;
    for (auto& cell : cells_) cell.position_ = cell.position_ - center;
}
void Organism::removeDead(NutrientField& nutrients) {
    bool removed = false;
    for (const auto& cell : cells_) {
        if (!cell.alive()) {
            nutrients.deposit(worldPosition(cell), cell.energy() * 0.5f + 8.f);
            removed = true;
        }
    }
    if (removed) {
        std::erase_if(cells_, [](const Cell& cell) { return !cell.alive(); });
        recenter();
        position_ = wrap(position_, nutrients.extent());
    }
}
}
