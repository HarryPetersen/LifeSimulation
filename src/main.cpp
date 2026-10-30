#include "life/Renderer.hpp"
#include "life/Simulation.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        sf::RenderWindow window(sf::VideoMode({1000, 750}), "Life Simulation");
        window.setVerticalSyncEnabled(true);
        life::Simulation simulation;
        life::Renderer renderer;
        sf::Clock clock;
        bool paused = false;
        bool nutrients = true;
        float speed = 1.f;
        float titleTimer = 0.f;
        while (window.isOpen()) {
            const float elapsed = clock.restart().asSeconds();
            while (const auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) window.close();
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    switch (key->code) {
                    case sf::Keyboard::Key::Escape: window.close(); break;
                    case sf::Keyboard::Key::Space: paused = !paused; break;
                    case sf::Keyboard::Key::R: simulation.reset(); break;
                    case sf::Keyboard::Key::N: nutrients = !nutrients; break;
                    case sf::Keyboard::Key::Right: if (paused) simulation.step(); break;
                    case sf::Keyboard::Key::Up: speed = std::min(4.f, speed * 2.f); break;
                    case sf::Keyboard::Key::Down: speed = std::max(0.25f, speed * 0.5f); break;
                    default: break;
                    }
                }
                if (const auto* resized = event->getIf<sf::Event::Resized>(); resized && resized->size.x && resized->size.y) {
                    sf::View view(sf::FloatRect({0.f, 0.f}, {1000.f, 750.f}));
                    const float aspect = static_cast<float>(resized->size.x) / static_cast<float>(resized->size.y);
                    const float worldAspect = 1000.f / 750.f;
                    if (aspect > worldAspect) {
                        const float width = worldAspect / aspect;
                        view.setViewport(sf::FloatRect({(1.f - width) * 0.5f, 0.f}, {width, 1.f}));
                    } else {
                        const float height = aspect / worldAspect;
                        view.setViewport(sf::FloatRect({0.f, (1.f - height) * 0.5f}, {1.f, height}));
                    }
                    window.setView(view);
                }
            }
            if (!window.isOpen()) break;
            if (!paused) simulation.advance(static_cast<double>(elapsed) * speed);
            titleTimer += elapsed;
            if (titleTimer >= 0.25f) {
                const auto stats = simulation.stats();
                window.setTitle("Life | Organisms " + std::to_string(stats.organisms) + " | Cells " + std::to_string(stats.cells) +
                    " | Generation " + std::to_string(stats.highestGeneration) + " | " + std::to_string(speed) +
                    "x | " + (paused ? "Paused" : "Running") + " | Space pause, R reset, N nutrients, arrows speed/step");
                titleTimer = 0.f;
            }
            window.clear(sf::Color{8, 12, 20});
            renderer.draw(window, simulation, nutrients);
            window.display();
        }
    } catch (const std::exception& error) {
        std::cerr << "Simulation error: " << error.what() << '\n';
        return 1;
    }
}
