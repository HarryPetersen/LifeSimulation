#pragma once

#include "life/SpatialGrid.hpp"
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace life {
struct AdhesionBond {
    std::uint64_t firstCell{};
    std::uint64_t secondCell{};
    float restLength{};
    float strength{};
    float age{};
};

class InteractionSystem {
public:
    void update(std::span<Organism> organisms, Vec2 extent, float dt);
    void prune(std::span<const Organism> organisms);
    void reset();
    [[nodiscard]] std::span<const AdhesionBond> bonds() const noexcept { return bonds_; }
    [[nodiscard]] std::size_t contacts() const noexcept { return contacts_; }
private:
    void index(std::span<const Organism> organisms);
    void solveBonds(std::span<Organism> organisms, Vec2 extent, float dt);
    void collide(Organism& first, Cell& a, Organism& second, Cell& b, Vec2 extent);
    void interact(Organism& first, Cell& a, Organism& second, Cell& b, float dt);
    void sense(std::span<Organism> organisms, Vec2 extent);
    static void attack(Cell& predator, Cell& prey, float dt);
    SpatialGrid grid_;
    std::unordered_map<std::uint64_t, CellAddress> addresses_;
    std::vector<AdhesionBond> bonds_;
    std::size_t contacts_{};
};
}
