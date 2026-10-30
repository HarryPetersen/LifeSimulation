#include "life/InteractionSystem.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace life {
void InteractionSystem::reset() {
    bonds_.clear();
    addresses_.clear();
    contacts_ = 0;
}
void InteractionSystem::index(std::span<const Organism> organisms) {
    addresses_.clear();
    for (std::size_t i = 0; i < organisms.size(); ++i) {
        const auto cells = organisms[i].cells();
        for (std::size_t j = 0; j < cells.size(); ++j)
            if (cells[j].alive()) addresses_.emplace(cells[j].id(), CellAddress{i, j, {}});
    }
}
void InteractionSystem::prune(std::span<const Organism> organisms) {
    index(organisms);
    std::erase_if(bonds_, [&](const AdhesionBond& bond) {
        const auto first = addresses_.find(bond.firstCell), second = addresses_.find(bond.secondCell);
        return first == addresses_.end() || second == addresses_.end() || first->second.organism == second->second.organism;
    });
}
void InteractionSystem::attack(Cell& predator, Cell& prey, float dt) {
    if (!predator.alive() || !prey.alive() || predator.type() != CellType::Predator) return;
    const float demand = dt * 2.f;
    const float effort = predator.withdraw(demand) / demand;
    const float damage = predator.genome().attack * dt * effort;
    prey.damage(damage);
    const float resistance = prey.type() == CellType::Defensive ? 0.25f : 1.f;
    const float amount = std::min(predator.capacity() - predator.energy(), damage * 1.5f * resistance);
    predator.receive(prey.withdraw(amount));
    if (prey.type() == CellType::Defensive && prey.alive()) {
        const float counter = prey.withdraw(dt) / dt;
        predator.damage(damage * 0.5f * counter);
    }
}
void InteractionSystem::interact(Organism& first, Cell& a, Organism& second, Cell& b, float dt) {
    if (!a.alive() || !b.alive()) return;
    if (first.lineage() != second.lineage()) {
        attack(a, b, dt);
        attack(b, a, dt);
    }
    if (!a.alive() || !b.alive() || a.type() == CellType::Predator || b.type() == CellType::Predator) return;
    if (a.type() != CellType::Adhesive && b.type() != CellType::Adhesive) return;
    if (a.energy() < 10.f || b.energy() < 10.f || bonds_.size() >= 512) return;
    std::size_t aCount = 0, bCount = 0;
    for (const auto& bond : bonds_) {
        if ((bond.firstCell == a.id() && bond.secondCell == b.id()) ||
            (bond.firstCell == b.id() && bond.secondCell == a.id())) return;
        aCount += static_cast<std::size_t>(bond.firstCell == a.id() || bond.secondCell == a.id());
        bCount += static_cast<std::size_t>(bond.firstCell == b.id() || bond.secondCell == b.id());
    }
    if (aCount >= 2 || bCount >= 2) return;
    a.withdraw(1.f);
    b.withdraw(1.f);
    bonds_.push_back({a.id(), b.id(), a.radius() + b.radius() + 1.f,
                      std::min(a.genome().adhesion, b.genome().adhesion), 0.f});
}
void InteractionSystem::solveBonds(std::span<Organism> organisms, Vec2 extent, float dt) {
    std::erase_if(bonds_, [&](AdhesionBond& bond) {
        const auto ai = addresses_.find(bond.firstCell), bi = addresses_.find(bond.secondCell);
        if (ai == addresses_.end() || bi == addresses_.end()) return true;
        auto& first = organisms[ai->second.organism];
        auto& second = organisms[bi->second.organism];
        auto& a = first.cells_[ai->second.cell];
        auto& b = second.cells_[bi->second.cell];
        if (!a.alive() || !b.alive() || first.id() == second.id()) return true;
        const Vec2 delta = displacement(first.worldPosition(a), second.worldPosition(b), extent);
        const float distance = delta.length();
        bond.restLength = a.radius() + b.radius() + 1.f;
        bond.age += dt;
        const float upkeep = dt * 0.15f;
        if (distance > bond.restLength + 18.f * bond.strength || a.energy() < upkeep || b.energy() < upkeep) return true;
        a.withdraw(upkeep);
        b.withdraw(upkeep);
        const Vec2 normal = delta.normalized();
        const Vec2 armA = rotate(a.position(), first.angle_), armB = rotate(b.position(), second.angle_);
        const float separating = dot(second.pointVelocity(armB) - first.pointVelocity(armA), normal);
        const float force = std::clamp((distance - bond.restLength) * 12.f * bond.strength + separating * 2.f, -15.f, 15.f);
        first.impulse(normal * (force * dt), armA);
        second.impulse(normal * (-force * dt), armB);
        const float difference = a.energy() / a.capacity() - b.energy() / b.capacity();
        auto& donor = difference > 0.f ? a : b;
        auto& recipient = difference > 0.f ? b : a;
        const float amount = std::min({std::abs(difference) * 8.f * dt, donor.energy(), recipient.capacity() - recipient.energy()});
        recipient.receive(donor.withdraw(amount));
        if (a.distress() > 0.1f) second.externalCue_ += normal * (a.distress() * dt * 2.f);
        if (b.distress() > 0.1f) first.externalCue_ += normal * (-b.distress() * dt * 2.f);
        return false;
    });
}
void InteractionSystem::collide(Organism& first, Cell& a, Organism& second, Cell& b, Vec2 extent) {
    if (!a.alive() || !b.alive()) return;
    const Vec2 delta = displacement(first.worldPosition(a), second.worldPosition(b), extent);
    const float distance = delta.length();
    const float overlap = a.radius() + b.radius() - distance;
    if (overlap <= 0.f) return;
    const Vec2 normal = distance > 0.0001f ? delta * (1.f / distance) : Vec2{a.id() < b.id() ? 1.f : -1.f, 0.f};
    const Vec2 armA = rotate(a.position(), first.angle_) + normal * a.radius();
    const Vec2 armB = rotate(b.position(), second.angle_) - normal * b.radius();
    const float inverseA = first.inverseMass(), inverseB = second.inverseMass();
    const float inertiaA = first.inverseInertia(), inertiaB = second.inverseInertia();
    const auto effectiveMass = [&](Vec2 axis) {
        const float ca = cross(armA, axis), cb = cross(armB, axis);
        return inverseA + inverseB + ca * ca * inertiaA + cb * cb * inertiaB;
    };
    const Vec2 relative = second.pointVelocity(armB) - first.pointVelocity(armA);
    const float closing = dot(relative, normal);
    if (closing < 0.f) {
        const float normalImpulse = -1.1f * closing / effectiveMass(normal);
        first.impulse(normal * -normalImpulse, armA);
        second.impulse(normal * normalImpulse, armB);
        const Vec2 tangent = perpendicular(normal);
        const float friction = std::clamp(-dot(relative, tangent) / effectiveMass(tangent), -normalImpulse * 0.35f, normalImpulse * 0.35f);
        first.impulse(tangent * -friction, armA);
        second.impulse(tangent * friction, armB);
        const float trauma = std::max(0.f, -closing - 12.f) * 0.08f;
        a.damage(trauma);
        b.damage(trauma);
    }
    const float correction = std::max(0.f, overlap - 0.01f) * 0.65f / (inverseA + inverseB);
    first.position_ = wrap(first.position_ - normal * (correction * inverseA), extent);
    second.position_ = wrap(second.position_ + normal * (correction * inverseB), extent);
}
void InteractionSystem::sense(std::span<Organism> organisms, Vec2 extent) {
    grid_.rebuild(organisms, extent);
    const auto entries = grid_.entries();
    for (const auto& entry : entries) {
        auto& body = organisms[entry.organism];
        const auto& sensor = body.cells_[entry.cell];
        if (sensor.type() != CellType::Sensory && sensor.type() != CellType::Predator) continue;
        Vec2 cue{};
        float weight = 0.f;
        for (auto candidate : grid_.query(entry.position, sensor.genome().sensing)) {
            const auto& other = entries[candidate];
            if (other.organism == entry.organism) continue;
            const auto& neighbor = organisms[other.organism];
            const auto& target = neighbor.cells_[other.cell];
            const Vec2 delta = displacement(entry.position, other.position, extent);
            const float distance = delta.length();
            if (distance < 0.001f) continue;
            const float strength = 1.f - distance / sensor.genome().sensing;
            float response = 0.f;
            if (sensor.type() == CellType::Predator && body.lineage() != neighbor.lineage()) response = 2.f;
            else if (target.type() == CellType::Predator && body.lineage() != neighbor.lineage()) response = -3.f;
            else if (target.distress() > 0.1f) response = -target.distress();
            cue += delta * (response * strength / distance);
            weight += strength;
        }
        if (weight > 0.f) body.externalCue_ += cue * (1.f / weight);
    }
}
void InteractionSystem::update(std::span<Organism> organisms, Vec2 extent, float dt) {
    if (!std::isfinite(dt) || dt <= 0.f || dt > 0.1f || !finite(extent) || extent.x <= 0.f || extent.y <= 0.f)
        throw std::invalid_argument("Invalid interaction step");
    prune(organisms);
    std::set<std::pair<std::uint64_t, std::uint64_t>> contacted;
    std::set<std::pair<std::uint64_t, std::uint64_t>> interacted;
    const int substeps = std::max(2, static_cast<int>(std::ceil(dt / (1.f / 120.f))));
    const float substep = dt / static_cast<float>(substeps);
    for (int step = 0; step < substeps; ++step) {
        solveBonds(organisms, extent, substep);
        for (auto& organism : organisms) organism.integrate(substep, extent);
        for (int iteration = 0; iteration < 4; ++iteration) {
            grid_.rebuild(organisms, extent);
            const auto entries = grid_.entries();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                const auto& first = entries[i];
                for (auto j : grid_.query(first.position, 30.f)) {
                    const auto& second = entries[j];
                    if (j <= i || first.organism == second.organism) continue;
                    auto& bodyA = organisms[first.organism];
                    auto& bodyB = organisms[second.organism];
                    auto& a = bodyA.cells_[first.cell];
                    auto& b = bodyB.cells_[second.cell];
                    if (!a.alive() || !b.alive()) continue;
                    const float distance = displacement(bodyA.worldPosition(a), bodyB.worldPosition(b), extent).length();
                    const std::pair<std::uint64_t, std::uint64_t> pair{std::min(a.id(), b.id()), std::max(a.id(), b.id())};
                    if (distance <= a.radius() + b.radius() + 2.f && interacted.insert(pair).second)
                        interact(bodyA, a, bodyB, b, dt);
                    if (distance < a.radius() + b.radius()) {
                        contacted.insert(pair);
                        collide(bodyA, a, bodyB, b, extent);
                    }
                }
            }
        }
    }
    contacts_ = contacted.size();
    sense(organisms, extent);
    prune(organisms);
}
}
