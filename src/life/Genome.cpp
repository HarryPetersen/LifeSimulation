#include "life/Genome.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace life {
void Genome::validate() const {
    const auto valid = [](float value, float low, float high) {
        return std::isfinite(value) && value >= low && value <= high;
    };
    if (!valid(radius, 3.f, 12.f) || !valid(metabolism, 0.1f, 3.f) ||
        !valid(uptake, 0.5f, 10.f) || !valid(thrust, 5.f, 90.f) ||
        !valid(sensing, 10.f, 120.f) || !valid(lifespan, 30.f, 600.f) ||
        !valid(divisionAge, 3.f, 60.f) || !valid(mutationRate, 0.f, 1.f) || !valid(attack, 1.f, 30.f) || !valid(adhesion, 0.1f, 1.f)) {
        throw std::invalid_argument("Genome trait outside supported range");
    }
}
Genome Genome::mutated(std::mt19937& random) const {
    validate();
    Genome result = *this;
    std::bernoulli_distribution mutate(mutationRate);
    std::normal_distribution<float> scale(1.f, 0.12f);
    const auto change = [&](float& value, float low, float high) {
        if (mutate(random)) value = std::clamp(value * scale(random), low, high);
    };
    change(result.radius, 3.f, 12.f);
    change(result.metabolism, 0.1f, 3.f);
    change(result.uptake, 0.5f, 10.f);
    change(result.thrust, 5.f, 90.f);
    change(result.sensing, 10.f, 120.f);
    change(result.lifespan, 30.f, 600.f);
    change(result.divisionAge, 3.f, 60.f);
    change(result.mutationRate, 0.f, 1.f);
    change(result.attack, 1.f, 30.f);
    change(result.adhesion, 0.1f, 1.f);
    return result;
}
}
