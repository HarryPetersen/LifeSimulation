#pragma once

#include "life/Organism.hpp"
#include <cstddef>
#include <span>
#include <vector>

namespace life {
struct CellAddress {
    std::size_t organism{};
    std::size_t cell{};
    Vec2 position{};
};

class SpatialGrid {
public:
    void rebuild(std::span<const Organism> organisms, Vec2 extent);
    [[nodiscard]] std::vector<std::size_t> query(Vec2 position, float radius) const;
    [[nodiscard]] std::span<const CellAddress> entries() const noexcept { return entries_; }
private:
    Vec2 extent_{1.f, 1.f};
    std::size_t columns_{1};
    std::size_t rows_{1};
    std::vector<CellAddress> entries_;
    std::vector<std::vector<std::size_t>> buckets_;
};
}
