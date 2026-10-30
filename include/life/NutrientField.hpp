#pragma once

#include "life/Vector.hpp"
#include <cstddef>
#include <span>
#include <vector>

namespace life {
class NutrientField {
public:
    NutrientField(Vec2 extent, std::size_t columns = 40, std::size_t rows = 30);
    void update(float dt);
    [[nodiscard]] float sample(Vec2 position) const;
    [[nodiscard]] Vec2 gradient(Vec2 position, float distance) const;
    float consume(Vec2 position, float amount);
    void deposit(Vec2 position, float amount);
    [[nodiscard]] Vec2 extent() const noexcept { return extent_; }
    [[nodiscard]] std::size_t columns() const noexcept { return columns_; }
    [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
    [[nodiscard]] std::span<const float> values() const noexcept { return values_; }
    static constexpr float capacity = 30.f;
private:
    [[nodiscard]] std::size_t index(Vec2 position) const;
    Vec2 extent_;
    std::size_t columns_;
    std::size_t rows_;
    std::vector<float> values_;
    std::vector<float> buffer_;
};
}
