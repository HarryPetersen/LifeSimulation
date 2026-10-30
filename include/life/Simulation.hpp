#pragma once

#include "life/SimulationStats.hpp"
#include "life/InteractionSystem.hpp"
#include "life/Organism.hpp"
#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace life {
class Simulation {
public:
    [[nodiscard]] SimulationStats stats() const noexcept;
    explicit Simulation(std::uint32_t seed = 42, Vec2 extent = {1000.f, 750.f}, std::size_t population = 12);
    void advance(double elapsed);
    void step();
    void reset(std::uint32_t seed = 42);
    [[nodiscard]] const NutrientField& nutrients() const noexcept { return nutrients_; }
    [[nodiscard]] std::span<const Organism> organisms() const noexcept { return organisms_; }
    [[nodiscard]] const InteractionSystem& interactions() const noexcept { return interactions_; }
    [[nodiscard]] float light() const noexcept;
    static constexpr double fixedStep = 1.0 / 60.0;
    static constexpr std::size_t maxOrganisms = 128;
private:
    void populate();
    NutrientField nutrients_;
    InteractionSystem interactions_;
    std::mt19937 random_;
    std::vector<Organism> organisms_;
    std::size_t initialPopulation_;
    std::uint64_t nextCellId_{1};
    std::uint64_t nextOrganismId_{1};
    std::uint64_t ticks_{};
    double accumulator_{};
};
}
