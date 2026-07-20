#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include "line_maker/line_maker.hh"
#include "line_maker/item_loader.hpp"
#include "line_maker/renderer.hpp"

void game_tick(std::vector<item_loader::mesh_actor>& actor_vec) {
    for (auto& act : actor_vec) {
        act.tick();
    }
}

void submit_actors(std::vector<item_loader::mesh_actor>& actor_vec, renderer::renderer& render_obj) {
    for (auto& act : actor_vec) {
        // Construct the payload with vertices, triangles, and a custom actor color
        renderer::RenderPayload payload;
        auto mesh_data = act.draw_self();
        
        payload.vertices = std::move(mesh_data.first);
        payload.triangles = std::move(mesh_data.second);
        payload.base_color = sf::Color(220, 100, 50); // Vibrant Copper/Orange Teapot!

        render_obj.submit(std::move(payload));
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "SFML 3D Light Engine");
    
    renderer::renderer my_renderer;
    renderer::Camera main_camera;

    // Create a sun light coming down from the top-right-front
    renderer::DirectionalLight sun_light;
    sun_light.dir_x = -0.5f;
    sun_light.dir_y = -1.0f;
    sun_light.dir_z = 0.5f;
    sun_light.ambient = 0.25f; // Keep shadow side softly lit

    main_camera.x = 0.0f;
    main_camera.y = 0.0f;
    main_camera.z = 0.0f; 

    std::vector<item_loader::mesh_actor> actor_store;
    item_loader::mesh_actor teapot("assets/deet.obj", 0.0f, 0.0f, 5.0f);
    actor_store.push_back(teapot);

    const float move_speed = 0.5f;
    const float rot_speed = 0.03f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // --- Camera Rotation (Arrow Keys) ---
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  main_camera.yaw -= rot_speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) main_camera.yaw += rot_speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    main_camera.pitch -= rot_speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  main_camera.pitch += rot_speed;

        const float max_pitch = 1.48f;
        if (main_camera.pitch > max_pitch)  main_camera.pitch = max_pitch;
        if (main_camera.pitch < -max_pitch) main_camera.pitch = -max_pitch;

        // --- Directional Movement (WASD) ---
        float forward_x =  std::sin(main_camera.yaw);
        float forward_z =  std::cos(main_camera.yaw);
        float right_x   =  std::cos(main_camera.yaw);
        float right_z   = -std::sin(main_camera.yaw);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            main_camera.x += forward_x * move_speed;
            main_camera.z += forward_z * move_speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            main_camera.x -= forward_x * move_speed;
            main_camera.z -= forward_z * move_speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            main_camera.x -= right_x * move_speed;
            main_camera.z -= right_z * move_speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            main_camera.x += right_x * move_speed;
            main_camera.z += right_z * move_speed;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))  main_camera.y += move_speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) main_camera.y -= move_speed;

        // --- Frame Pass ---
        game_tick(actor_store);
        submit_actors(actor_store, my_renderer);

        window.clear(sf::Color(30, 30, 30)); // Dark grey background
        my_renderer.render(window, main_camera, sun_light);
        window.display();
    }

    return 0;
}