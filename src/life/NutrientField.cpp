#include "life/NutrientField.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace life {
NutrientField::NutrientField(Vec2 extent, std::size_t columns, std::size_t rows)
    : extent_(extent), columns_(columns), rows_(rows) {
    if (!finite(extent) || extent.x <= 0.f || extent.y <= 0.f ||
        columns == 0 || rows == 0 || columns > 1024 || rows > 1024) {
        throw std::invalid_argument("Invalid nutrient field dimensions");
    }
    values_.resize(columns * rows);
    buffer_.resize(values_.size());
    for (std::size_t y = 0; y < rows; ++y) {
        for (std::size_t x = 0; x < columns; ++x) {
            values_[y * columns + x] = 15.f + 10.f * std::sin(static_cast<float>(x) * 0.4f) *
                                                        std::cos(static_cast<float>(y) * 0.3f);
        }
    }
}
std::size_t NutrientField::index(Vec2 position) const {
    if (!finite(position)) throw std::invalid_argument("Non-finite position");
    position = wrap(position, extent_);
    const auto x = std::min(columns_ - 1, static_cast<std::size_t>(position.x / extent_.x * static_cast<float>(columns_)));
    const auto y = std::min(rows_ - 1, static_cast<std::size_t>(position.y / extent_.y * static_cast<float>(rows_)));
    return y * columns_ + x;
}
float NutrientField::sample(Vec2 position) const { return values_[index(position)]; }
Vec2 NutrientField::gradient(Vec2 position, float distance) const {
    if (!std::isfinite(distance) || distance <= 0.f) throw std::invalid_argument("Invalid sensing distance");
    return Vec2{sample(position + Vec2{distance, 0.f}) - sample(position - Vec2{distance, 0.f}),
                sample(position + Vec2{0.f, distance}) - sample(position - Vec2{0.f, distance})}.normalized();
}
float NutrientField::consume(Vec2 position, float amount) {
    if (!std::isfinite(amount) || amount < 0.f) throw std::invalid_argument("Invalid nutrient amount");
    auto& value = values_[index(position)];
    const float taken = std::min(value, amount);
    value -= taken;
    return taken;
}
void NutrientField::deposit(Vec2 position, float amount) {
    if (!std::isfinite(amount) || amount < 0.f) throw std::invalid_argument("Invalid nutrient amount");
    auto& value = values_[index(position)];
    value = std::min(capacity, value + amount);
}
void NutrientField::update(float dt) {
    if (!std::isfinite(dt) || dt < 0.f || dt > 0.1f) throw std::invalid_argument("Field step must be in [0, 0.1]");
    for (std::size_t y = 0; y < rows_; ++y) {
        for (std::size_t x = 0; x < columns_; ++x) {
            const auto i = y * columns_ + x;
            const float adjacent = values_[y * columns_ + (x + 1) % columns_] +
                values_[y * columns_ + (x + columns_ - 1) % columns_] +
                values_[((y + 1) % rows_) * columns_ + x] +
                values_[((y + rows_ - 1) % rows_) * columns_ + x];
            buffer_[i] = std::clamp(values_[i] + dt * (0.6f * (adjacent - 4.f * values_[i]) +
                                    0.12f * (capacity - values_[i])), 0.f, capacity);
        }
    }
    values_.swap(buffer_);
}
}
