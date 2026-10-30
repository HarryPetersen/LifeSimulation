#include "life/Simulation.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void near(float first, float second, const std::string& message) {
    check(std::abs(first - second) < 0.001f, message);
}
template<class Action> void rejects(Action action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "Expected invalid_argument");
}
void field() {
    life::NutrientField field({200.f, 150.f}, 8, 6);
    near(field.sample({0.f, 0.f}), field.sample({200.f, -150.f}), "Toroidal sampling");
    const float initial = field.sample({0.f, 0.f});
    near(field.consume({0.f, 0.f}, 1000.f), initial, "Consumption bounded by availability");
    near(field.sample({0.f, 0.f}), 0.f, "Depleted field");
    field.deposit({0.f, 0.f}, 1000.f);
    near(field.sample({0.f, 0.f}), life::NutrientField::capacity, "Deposit capacity");
    field.consume({0.f, 0.f}, 1000.f);
    field.update(0.1f);
    check(field.sample({0.f, 0.f}) > 0.f, "Diffusion and regeneration replenish patch");
    for (int i = 0; i < 100; ++i) field.update(0.1f);
    for (float value : field.values()) check(value >= 0.f && value <= life::NutrientField::capacity, "Field bounds");
    rejects([&] { field.consume({}, -1.f); });
    rejects([&] { field.update(1.f); });
    rejects([&] { (void)field.sample({std::numeric_limits<float>::infinity(), 0.f}); });
    rejects([] { life::NutrientField invalid({0.f, 100.f}); });
}
void lifecycle() {
    life::NutrientField field({200.f, 200.f});
    life::Cell cell(1, life::CellType::General, {});
    std::mt19937 random(7);
    for (int i = 0; i < 1000 && !cell.readyToDivide(); ++i) {
        cell.receive(5.f);
        cell.update(0.1f, {}, field, 1.f);
    }
    check(cell.readyToDivide(), "Fed mature cell becomes division ready");
    const float energy = cell.energy();
    auto child = cell.divide(2, {26.f, 0.f}, random);
    check(child.has_value(), "Division produces daughter");
    near(cell.energy() + child->energy(), energy - 12.f, "Division conserves energy minus cost");
    check(child->id() == 2 && child->age() == 0.f, "Daughter identity and age");
    check(!cell.divide(3, {}, random), "Division cooldown");
    cell.damage(1000.f);
    check(!cell.alive(), "Lethal damage");
    near(cell.receive(100.f), 0.f, "Dead cells cannot absorb energy");
    const float age = cell.age();
    cell.update(0.1f, {}, field, 1.f);
    near(cell.age(), age, "Dead cell does not age");
    life::Genome fast;
    fast.lifespan = 30.f;
    life::Cell ageing(9, life::CellType::Photosynthetic, {}, fast);
    for (int i = 0; i < 600; ++i) { ageing.receive(5.f); ageing.update(0.1f, {}, field, 1.f); }
    check(!ageing.alive(), "Senescence kills well-fed cells");
    life::Genome starvingGenome;
    starvingGenome.metabolism = 3.f;
    starvingGenome.uptake = 0.5f;
    life::Cell starving(10, life::CellType::General, {}, starvingGenome);
    starving.withdraw(100.f);
    for (int i = 0; i < 100; ++i) {
        field.consume({}, 1000.f);
        starving.update(0.1f, {}, field, 0.f);
    }
    check(!starving.alive(), "Starvation kills depleted cells");
}
void specialization() {
    life::NutrientField field({200.f, 200.f});
    life::Cell defense(1, life::CellType::Defensive, {});
    life::Cell general(2, life::CellType::General, {});
    defense.damage(10.f);
    general.damage(10.f);
    check(defense.health() > general.health(), "Defensive damage resistance");
    life::Cell storage(3, life::CellType::Storage, {});
    check(storage.capacity() > general.capacity(), "Storage capacity");
    life::Cell lit(4, life::CellType::Photosynthetic, {});
    life::Cell dark(5, life::CellType::Photosynthetic, {});
    field.consume({}, 1000.f);
    lit.update(0.1f, {}, field, 1.f);
    dark.update(0.1f, {}, field, 0.f);
    check(lit.energy() > dark.energy(), "Photosynthesis responds to light");
    life::Genome stable;
    stable.mutationRate = 0.f;
    std::mt19937 random(9);
    near(stable.mutated(random).radius, stable.radius, "Zero mutation rate");
    stable.mutationRate = 1.f;
    for (int i = 0; i < 1000; ++i) { stable = stable.mutated(random); stable.validate(); }
}
void organism() {
    life::NutrientField field({300.f, 300.f});
    std::mt19937 random(2);
    std::uint64_t next = 20;
    std::vector<life::Cell> cells;
    cells.emplace_back(1, life::CellType::Movement, life::Vec2{});
    life::Organism mobile(1, {299.f, 100.f}, std::move(cells));
    for (int i = 0; i < 100; ++i) mobile.update(0.1f, field, 1.f, random, next);
    check(mobile.velocity().length() > 0.f, "Movement cells produce thrust");
    check(mobile.position().x >= 0.f && mobile.position().x < 300.f, "Organism wraps at world edge");
    std::vector<life::Cell> dead;
    dead.emplace_back(9, life::CellType::General, life::Vec2{});
    dead.front().damage(1000.f);
    life::Organism empty(2, {}, std::move(dead));
    const float before = field.sample({});
    empty.update(0.1f, field, 1.f, random, next);
    check(!empty.alive(), "Dead cells removed");
    check(field.sample({}) >= before, "Death recycles nutrients");
    const float recycled = field.sample({});
    empty.update(0.1f, field, 1.f, random, next);
    near(field.sample({}), recycled, "Death recycles exactly once");
}
void reproduction() {
    life::NutrientField field({500.f, 500.f});
    std::mt19937 random(12);
    std::uint64_t next = 100;
    life::Genome genome;
    genome.divisionAge = 60.f;
    std::vector<life::Cell> cells;
    for (std::uint64_t i = 0; i < 12; ++i)
        cells.emplace_back(i + 1, life::CellType::Photosynthetic,
            life::Vec2{static_cast<float>(i % 4) * 26.f, static_cast<float>(i / 4) * 26.f}, genome);
    life::Organism parent(1, {100.f, 100.f}, std::move(cells));
    check(!parent.reproduce(2, field.extent()), "Organism reproduction cooldown");
    for (int i = 0; i < 210; ++i) parent.update(0.1f, field, 1.f, random, next);
    const auto count = parent.cells().size();
    const float energy = parent.energy();
    auto child = parent.reproduce(2, field.extent());
    check(child.has_value(), "Mature colony reproduces");
    check(child->generation() == 1 && child->id() == 2, "Offspring lineage");
    check(parent.cells().size() + child->cells().size() == count, "Fission conserves cells");
    near(parent.energy() + child->energy(), energy, "Fission conserves energy");
    check(!child->reproduce(3, field.extent()), "Offspring cooldown");
}
void invariants(const life::Simulation& simulation) {
    std::unordered_set<std::uint64_t> ids;
    std::unordered_set<std::uint64_t> organismIds;
    check(simulation.organisms().size() <= life::Simulation::maxOrganisms, "Population bounded");
    for (const auto& organism : simulation.organisms()) {
        check(organismIds.insert(organism.id()).second, "Unique organism identity");
        check(organism.cells().size() <= life::Organism::maxCells, "Cell count bounded");
        check(life::finite(organism.position()) && life::finite(organism.velocity()), "Finite motion");
        for (const auto& cell : organism.cells()) {
            check(ids.insert(cell.id()).second, "Globally unique cell identity");
            check(cell.alive(), "Only living cells retained");
            check(std::isfinite(cell.energy()) && cell.energy() >= 0.f && cell.energy() <= cell.capacity(), "Energy bounds");
            check(cell.health() > 0.f && cell.health() <= 100.f, "Health bounds");
            cell.genome().validate();
        }
    }
}
void simulation() {
    life::Simulation first(42), second(42);
    for (int i = 0; i < 3600; ++i) { first.step(); second.step(); }
    const auto a = first.stats();
    const auto b = second.stats();
    check(a.cells == b.cells && a.organisms == b.organisms, "Seeded population repeatability");
    near(a.totalEnergy, b.totalEnergy, "Seeded energy repeatability");
    check(a.highestGeneration > 0, "Organisms reproduce during integration run");
    invariants(first);
    first.reset(42);
    life::Simulation fresh(42);
    near(first.organisms()[0].position().x, fresh.organisms()[0].position().x, "Reset restores random seed");
    check(first.stats().elapsed == 0. && first.stats().cells == 72, "Reset restores initial population");
    first.advance(1.0 / 30.0);
    fresh.advance(1.0 / 60.0);
    fresh.advance(1.0 / 60.0);
    near(first.stats().totalEnergy, fresh.stats().totalEnergy, "Fixed step independent of frame grouping");
    near(first.organisms()[0].position().x, fresh.organisms()[0].position().x, "Fixed step motion");
    life::Simulation empty(1, {100.f, 100.f}, 0);
    empty.advance(100.);
    check(empty.stats().elapsed <= 0.251, "Catch-up bounded");
    rejects([&] { empty.advance(-1.); });
    rejects([&] { empty.advance(std::numeric_limits<double>::quiet_NaN()); });
    rejects([] { life::Simulation invalid(1, {100.f, 100.f}, 129); });
    for (int i = 0; i < 18000; ++i) {
        first.step();
        if (i % 600 == 0) invariants(first);
    }
    invariants(first);
}
}
int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"nutrient field", field}, {"cell lifecycle", lifecycle}, {"specialization and mutation", specialization},
        {"organism behavior", organism}, {"organism reproduction", reproduction}, {"simulation integration", simulation}
    };
    int failures = 0;
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
