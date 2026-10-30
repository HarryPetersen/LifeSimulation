# LifeSimulation

A C++20 cellular ecosystem with an SFML 3 frontend. Organisms are colonies of specialized cells competing for a regenerating nutrient field. The core has no graphics dependency and can run in tests or another frontend.

## Build

Requires CMake 3.22+, a C++20 compiler, and SFML 3 when building the graphical application.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/LifeSimulation` (or `build/Release/LifeSimulation.exe` with a multi-configuration Windows generator). If SFML is installed outside the normal search path, pass `-DCMAKE_PREFIX_PATH=/path/to/sfml` when configuring. The existing VS Code settings use MSYS2 UCRT64; run the build task with its `bin` directory on PATH. The task now builds every source through CMake.

Headless build:

```sh
cmake -S . -B build-headless -DLIFE_BUILD_APP=OFF
cmake --build build-headless
ctest --test-dir build-headless --output-on-failure
```

For GCC/Clang memory and undefined-behavior checks, add `-DLIFE_ENABLE_SANITIZERS=ON` to a separate Debug build.

## Controls

| Key | Action |
| --- | --- |
| Space | Pause or resume |
| Right | Advance one simulation tick while paused |
| Up / Down | Double / halve speed, between 0.25x and 4x |
| R | Reset to the initial seed |
| N | Toggle nutrient field visibility |
| Escape | Close |

The title shows organism count, cell count, highest offspring generation, speed, and pause state. Window resizing preserves the world's aspect ratio. Nutrient-rich patches are brighter. Cell nuclei brighten with energy; membranes darken with damage; division-ready cells have white outlines.

## Cell system

| Type | Color | Specialization |
| --- | --- | --- |
| General | Mint | Enhanced nutrient uptake |
| Movement | Blue | Converts energy into organism thrust |
| Sensory | Amber | Detects nutrient gradients and steers the colony |
| Photosynthetic | Green | Generates energy from the day/night light cycle |
| Storage | Purple | Increased energy capacity; shares reserves with neighbors |
| Defensive | Coral | Reduces incoming damage, including environmental damage |

Every cell owns its genome, stable identity, local position, age, health, energy, growth, and lifecycle phase. A genome contains bounded traits for size, metabolism, nutrient uptake, thrust, sensing range, lifespan, division timing, and mutation probability.

Cells absorb local nutrients, pay maintenance costs, repair damage when reserves permit, grow, and mature. Starvation and senescence cause damage. A mature cell with sufficient energy and health becomes division-ready. Division requires free space, spends 12 energy, splits remaining energy between parent and daughter, resets growth, and applies bounded mutations to the daughter. Specialization can also mutate. Parent age is retained; daughter age starts at zero.

Adjacent living cells exchange energy according to relative reserve fullness. Sensory cells combine local nutrient gradients into a heading; movement cells pay for thrust. Motion uses organism mass and exponential drag. Dead cells return part of their reserves and biomass to the field and are removed exactly once. Empty organisms disappear.

Colonies of at least 12 cells may split after a 20-second cooldown if their average reserve is at least 30. Fission transfers existing cells and their energy to an offspring organism with a new identity and incremented generation. Colony and global population limits bound simulation cost (24 cells per colony, 128 organisms). A blocked division waits for a later tick without spending energy or assigning an identity.

## Architecture

- `Vector.hpp`: graphics-independent vector and toroidal coordinate operations.
- `Genome.hpp/.cpp`: validated heritable traits and bounded mutation.
- `NutrientField.hpp/.cpp`: periodic grid, resource consumption/deposition, diffusion, regeneration, sensing.
- `Cell.hpp/.cpp`: cell state, metabolism, damage, growth, and division.
- `Organism.hpp/.cpp`: colony ownership, energy exchange, sensing, locomotion, growth, and fission.
- `Simulation.hpp/.cpp`: seeded randomness, stable identity allocation, fixed-step scheduling, population management, statistics.
- `Renderer.hpp/.cpp`: SFML presentation without simulation state mutation.
- `main.cpp`: window, input, timing, and application error reporting.
- `tests/SimulationTests.cpp`: headless lifecycle, specialization, resource, reproduction, repeatability, and long-run invariant checks.

Updates run at 60 fixed ticks per simulated second. `advance` accepts real elapsed seconds and caps catch-up at 0.25 seconds per call; stalls intentionally slow simulation time rather than produce an unbounded update backlog. `step` advances exactly one tick. Lower-level update functions accept finite steps from zero through 0.1 seconds. Public mutation boundaries reject invalid inputs.

A fixed seed reproduces a run on the same standard-library/compiler implementation and call sequence. Standard random distributions do not guarantee identical results across different C++ implementations. Reset restores the initial population, resource field, identities, clock, and random seed.

## Model boundaries

This is an artificial-life model, not a biochemical solver. Cells use rigid local coordinates within a colony; colonies may overlap and do not currently collide, attack, or form physical bonds. Fission partitions the cell collection and offsets the offspring, rather than simulating membrane mechanics. Nutrient diffusion operates on grid cells with periodic boundaries. Resource and population caps deliberately bound the sandbox; excess deposited nutrients are discarded at field capacity. State is in memory for the running session.
