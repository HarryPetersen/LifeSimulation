#include "life/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace life {
Simulation::Simulation(std::uint32_t seed, Vec2 extent, std::size_t population)
    : nutrients_(extent), random_(seed), initialPopulation_(population) {
    if (population > maxOrganisms) throw std::invalid_argument("Initial population exceeds limit");
    organisms_.reserve(maxOrganisms);
    populate();
}
void Simulation::populate() {
    std::uniform_real_distribution<float> x(0.f, nutrients_.extent().x);
    std::uniform_real_distribution<float> y(0.f, nutrients_.extent().y);
    for (std::size_t i = 0; i < initialPopulation_; ++i) {
        std::vector<Cell> cells;
        cells.emplace_back(nextCellId_++, CellType::General, Vec2{});
        cells.emplace_back(nextCellId_++, CellType::Movement, Vec2{-26.f, 0.f});
        cells.emplace_back(nextCellId_++, CellType::Sensory, Vec2{26.f, 0.f});
        cells.emplace_back(nextCellId_++, CellType::Photosynthetic, Vec2{0.f, -26.f});
        cells.emplace_back(nextCellId_++, CellType::Storage, Vec2{0.f, 26.f});
        cells.emplace_back(nextCellId_++, CellType::Defensive, Vec2{-26.f, 26.f});
        organisms_.emplace_back(nextOrganismId_++, Vec2{x(random_), y(random_)}, std::move(cells));
    }
}
void Simulation::reset(std::uint32_t seed) {
    nutrients_ = NutrientField(nutrients_.extent());
    random_.seed(seed);
    organisms_.clear();
    nextCellId_ = nextOrganismId_ = 1;
    ticks_ = 0;
    accumulator_ = 0.;
    populate();
}
float Simulation::light() const noexcept {
    return 0.55f + 0.45f * static_cast<float>(std::sin(static_cast<double>(ticks_) * fixedStep * 0.05));
}
void Simulation::advance(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0.) throw std::invalid_argument("Elapsed time must be finite and non-negative");
    accumulator_ += std::min(elapsed, 0.25);
    while (accumulator_ + 1e-12 >= fixedStep) {
        step();
        accumulator_ = std::max(0., accumulator_ - fixedStep);
    }
}
void Simulation::step() {
    const auto dt = static_cast<float>(fixedStep);
    nutrients_.update(dt);
    const float sunlight = light();
    for (auto& organism : organisms_) organism.update(dt, nutrients_, sunlight, random_, nextCellId_);
    std::erase_if(organisms_, [](const Organism& organism) { return !organism.alive(); });
    const auto existing = organisms_.size();
    for (std::size_t i = 0; i < existing && organisms_.size() < maxOrganisms; ++i) {
        if (auto child = organisms_[i].reproduce(nextOrganismId_, nutrients_.extent())) {
            ++nextOrganismId_;
            organisms_.push_back(std::move(*child));
        }
    }
    ++ticks_;
}
SimulationStats Simulation::stats() const noexcept {
    SimulationStats result;
    result.organisms = organisms_.size();
    result.elapsed = static_cast<double>(ticks_) * fixedStep;
    for (const auto& organism : organisms_) {
        result.cells += organism.cells().size();
        result.totalEnergy += organism.energy();
        result.highestGeneration = std::max(result.highestGeneration, organism.generation());
    }
    return result;
}
}
