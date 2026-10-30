#pragma once

#include <cstdint>
#include <random>

namespace life {
enum class CellType : std::uint8_t { General, Movement, Sensory, Photosynthetic, Storage, Defensive, Adhesive, Predator };

struct Genome {
    float radius{7.f};
    float metabolism{0.65f};
    float uptake{4.f};
    float thrust{35.f};
    float sensing{45.f};
    float lifespan{240.f};
    float divisionAge{12.f};
    float mutationRate{0.08f};

    float attack{10.f};
    float adhesion{0.6f};

    void validate() const;
    [[nodiscard]] Genome mutated(std::mt19937& random) const;
};
}
