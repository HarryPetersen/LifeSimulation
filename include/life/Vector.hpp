#pragma once

#include <cmath>

namespace life {
struct Vec2 {
    float x{};
    float y{};
    constexpr Vec2 operator+(Vec2 rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(Vec2 rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator*(float scale) const { return {x * scale, y * scale}; }
    constexpr Vec2& operator+=(Vec2 rhs) { x += rhs.x; y += rhs.y; return *this; }
    [[nodiscard]] float length() const { return std::hypot(x, y); }
    [[nodiscard]] Vec2 normalized() const {
        const float magnitude = length();
        return magnitude > 0.00001f ? *this * (1.f / magnitude) : Vec2{};
    }
};
inline bool finite(Vec2 value) { return std::isfinite(value.x) && std::isfinite(value.y); }
inline float wrap(float value, float extent) {
    const float result = std::fmod(value, extent);
    return result < 0.f ? result + extent : result;
}
inline Vec2 wrap(Vec2 value, Vec2 extent) { return {wrap(value.x, extent.x), wrap(value.y, extent.y)}; }
}
