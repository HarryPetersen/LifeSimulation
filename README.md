# LifeSimulation

A C++20 artificial-life simulation with an SFML 3 frontend. Specialized cells form colonies that grow, move, collide, cooperate, hunt, reproduce, and fragment when their structure is broken.

## Compile and run

Use a C++20 compiler and SFML 3. From an MSYS2 UCRT64 shell with SFML installed:

```sh
g++ -std=c++20 -O2 -Iinclude src/main.cpp src/life/*.cpp -o LifeSimulation.exe -lsfml-graphics -lsfml-window -lsfml-system
./LifeSimulation.exe
```

On Linux with SFML 3 available through pkg-config:

```sh
g++ -std=c++20 -O2 -Iinclude src/main.cpp src/life/*.cpp -o LifeSimulation $(pkg-config --cflags --libs sfml-graphics)
./LifeSimulation
```

Compile all the `.cpp` files, including the new `SpatialGrid.cpp` and `InteractionSystem.cpp`. For a custom SFML installation, add its include/library paths to the command and make its shared libraries available at runtime.

The remaining VS Code configuration is for IntelliSense only. It sets C++20, the project include path, and the original MSYS2 UCRT64 compiler location. Adjust `compilerPath` if your installation differs.

`life::Simulation::stats()` is declared publicly in `include/life/Simulation.hpp`, returns the self-contained `SimulationStats` type, and is implemented in `src/life/Simulation.cpp`. If VS Code still reports that this member is missing after updating the branch, run **C/C++: Reset IntelliSense Database**, reload the window, and ensure you opened this repository root. Check that your editor is using the included C++20 configuration and is not resolving an older copy of `Simulation.hpp`.

## Controls

| Key | Action |
| --- | --- |
| Space | Pause or resume |
| Right | Advance one tick while paused |
| Up / Down | Double / halve speed, from 0.25x to 4x |
| R | Reset the population, resources, bonds, identities, and random seed |
| N | Toggle nutrient visibility |
| Escape | Close |

The title shows organisms, cells, physical contacts during the last tick, active adhesive bonds, highest offspring generation, and speed. Resizing preserves the world aspect ratio.

## Cell roles

| Type | Color | Behavior |
| --- | --- | --- |
| General | Mint | Enhanced nutrient uptake |
| Movement | Blue | Pays energy to propel its colony |
| Sensory | Amber | Finds nutrients, avoids hostile predators and distress signals |
| Photosynthetic | Green | Generates energy from the day/night cycle |
| Storage | Purple | Holds larger reserves and shares with connected neighbors |
| Defensive | Coral | Resists damage and retaliates against attacking predators |
| Adhesive | Cyan | Forms physical connections with other non-predatory cells |
| Predator | Orange | Steers toward unrelated colonies, damages cells, and steals energy on contact |

Every cell has a stable identity, genome, position, health, energy, age, growth phase, and decaying distress signal. Genomes bound size, metabolism, uptake, thrust, sensing, lifespan, division timing, mutation rate, attack strength, and adhesion strength. Daughters can inherit mutated traits and specializations.

Cells consume local nutrients, pay maintenance, repair damage, grow, and divide when age, health, energy, and available space permit. Division spends energy and shares the remaining reserve between parent and daughter. Starvation, ageing, impact, and attacks cause damage. Dead cells recycle reserves and biomass exactly once before removal.

## Collisions and interactions

The periodic spatial grid finds nearby cell pairs across wraparound boundaries. Physical contacts use cell radii and colony mass/inertia. The solver separates penetrations, applies normal impulses and friction, transfers angular momentum, and damages cells on hard impacts. Physics uses substeps of at most 1/120 second with four contact passes each. Speed and angular velocity are bounded for stability.

Cells within a colony share a rigid structure. Local coordinates rotate with the colony; rendering, sensing, feeding, and collision detection all use the same world transform. A colony's center of mass is updated as cells grow, divide, or die while preserving cell world positions. Internal structural links are shown in muted gray.

Adhesive cells can join cells in other colonies through energy-funded spring/damper bonds. Each cell has at most two external bonds, and the world has at most 512. Bonds stretch, damp relative motion, share reserves, and transmit distress cues. They break when overextended, starved, or missing an endpoint. Cyan lines show active bonds. Endpoint identities remain valid through colony fission; dead endpoints are pruned.

Predators pay energy to attack unrelated cells in contact range. Victim defenses reduce damage and stolen energy; surviving defensive cells spend energy to retaliate. Founder lineage is inherited during reproduction and fragmentation, so descendants recognize kin. Sensory cells steer away from unrelated predators and nearby distress, while predators steer toward unrelated cells. Nutrient gradients continue to contribute to steering. Distressed cells have red outlines.

When dead cells disconnect the internal structure, connected components become separate colonies with preserved identities, world positions, velocity, and lineage. Reproduction spatially partitions a sufficiently large, well-fed colony into parent and offspring after a cooldown; it preserves cell state and reserves instead of teleporting an overlapping clone. New offspring increment generation, while fragments retain it.

## Modules

- `Vector`: vector algebra, rotation, and periodic displacement.
- `Genome`: validated traits and mutation.
- `Cell`: metabolism, growth, damage, distress, and division.
- `NutrientField`: diffusion, regeneration, consumption, and gradient sampling.
- `Organism`: cell ownership, structural connectivity, energy sharing, mass/inertia, motion, fission, and fragmentation.
- `SpatialGrid`: periodic spatial indexing and neighbor queries.
- `InteractionSystem`: contact resolution, predation, adhesion, resource exchange, and social sensing.
- `SimulationStats`: standalone public statistics data type.
- `Simulation`: population lifecycle, seeded randomness, fixed ticks, resources, and interactions.
- `Renderer`: SFML drawing of cells, internal links, adhesive bonds, health, energy, and nutrients.
- `main`: window, controls, timing, and application error handling.

## Simulation rules

Biology runs at 60 fixed ticks per simulated second. Real-time catch-up is capped at 0.25 seconds per frame. Population is bounded to 128 colonies with 24 cells each; excess growth and fragmentation wait for capacity. The nutrient field regenerates and caps stored resources. Energy sources are nutrients and light; metabolic costs, motion, attacks, repairs, division, and adhesive upkeep consume energy.

Runs with the same seed and update sequence are repeatable within the same C++ standard-library implementation. State is held in memory. This is a discrete artificial-life model: colonies are rigid bodies, and contact resolution is iterative rather than a soft-body membrane solver.
