#pragma once

#include <cstddef>
#include <cstdint>

namespace life {
struct SimulationStats {
    std::size_t organisms{};
    std::size_t cells{};
    std::size_t bonds{};
    std::size_t contacts{};
    std::uint32_t highestGeneration{};
    float totalEnergy{};
    double elapsed{};
};
}
