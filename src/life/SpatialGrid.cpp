#include "life/SpatialGrid.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace life {
void SpatialGrid::rebuild(std::span<const Organism> organisms, Vec2 extent) {
    if (!finite(extent) || extent.x <= 0.f || extent.y <= 0.f) throw std::invalid_argument("Invalid grid extent");
    extent_ = extent;
    columns_ = static_cast<std::size_t>(std::clamp(std::floor(extent.x / 32.f), 1.f, 256.f));
    rows_ = static_cast<std::size_t>(std::clamp(std::floor(extent.y / 32.f), 1.f, 256.f));
    buckets_.resize(columns_ * rows_);
    for (auto& bucket : buckets_) bucket.clear();
    entries_.clear();
    for (std::size_t body = 0; body < organisms.size(); ++body) {
        const auto cells = organisms[body].cells();
        for (std::size_t cell = 0; cell < cells.size(); ++cell) {
            if (!cells[cell].alive()) continue;
            const Vec2 point = wrap(organisms[body].worldPosition(cells[cell]), extent);
            const auto x = std::min(columns_ - 1, static_cast<std::size_t>(point.x / extent.x * static_cast<float>(columns_)));
            const auto y = std::min(rows_ - 1, static_cast<std::size_t>(point.y / extent.y * static_cast<float>(rows_)));
            buckets_[y * columns_ + x].push_back(entries_.size());
            entries_.push_back({body, cell, point});
        }
    }
}
std::vector<std::size_t> SpatialGrid::query(Vec2 position, float radius) const {
    if (!finite(position) || !std::isfinite(radius) || radius < 0.f) throw std::invalid_argument("Invalid spatial query");
    std::vector<std::size_t> result;
    if (buckets_.empty()) return result;
    position = wrap(position, extent_);
    const int columns = static_cast<int>(columns_), rows = static_cast<int>(rows_);
    const int x = std::min(columns - 1, static_cast<int>(position.x / extent_.x * static_cast<float>(columns)));
    const int y = std::min(rows - 1, static_cast<int>(position.y / extent_.y * static_cast<float>(rows)));
    const int reachX = static_cast<int>(std::min(static_cast<float>(columns), std::ceil(radius / (extent_.x / static_cast<float>(columns)))));
    const int reachY = static_cast<int>(std::min(static_cast<float>(rows), std::ceil(radius / (extent_.y / static_cast<float>(rows)))));
    const int width = std::min(columns, reachX * 2 + 1), height = std::min(rows, reachY * 2 + 1);
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            const auto bx = static_cast<std::size_t>((x - reachX + dx + columns) % columns);
            const auto by = static_cast<std::size_t>((y - reachY + dy + rows) % rows);
            for (auto entry : buckets_[by * columns_ + bx]) {
                if (displacement(position, entries_[entry].position, extent_).length() <= radius)
                    result.push_back(entry);
            }
        }
    }
    return result;
}
}
