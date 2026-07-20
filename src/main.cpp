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
        render_obj.submit(act.draw_self());
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "SFML 3D Engine");
    
    renderer::renderer my_renderer;
    renderer::Camera main_camera;

    main_camera.x = 0.0f;
    main_camera.y = 0.0f;
    main_camera.z = 0.0f; 

    std::vector<item_loader::mesh_actor> actor_store;
    item_loader::mesh_actor teapot("assets/deet.obj", 0.0f, 0.0f, 20.0f);
    actor_store.push_back(teapot);

    // Movement and rotation speeds
    const float move_speed = 0.5f;
    const float rot_speed = 0.03f; // Radians per tick (~1.7 degrees)

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // =====================================================================
        // 1. CAMERA ROTATION (Arrow Keys)
        // =====================================================================
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  main_camera.yaw -= rot_speed;  // Turn Left
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) main_camera.yaw += rot_speed;  // Turn Right
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    main_camera.pitch -= rot_speed; // Look Up
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  main_camera.pitch += rot_speed; // Look Down

        // Optional: Clamp pitch so the camera doesn't flip upside down (+/- 85 degrees)
        const float max_pitch = 1.48f; // ~85 degrees in radians
        if (main_camera.pitch > max_pitch)  main_camera.pitch = max_pitch;
        if (main_camera.pitch < -max_pitch) main_camera.pitch = -max_pitch;

        // =====================================================================
        // 2. DIRECTIONAL MOVEMENT (WASD) based on Camera Yaw
        // =====================================================================
        // Calculate forward and right unit vectors from current Yaw angle
        float forward_x =  std::sin(main_camera.yaw);
        float forward_z =  std::cos(main_camera.yaw);
        float right_x   =  std::cos(main_camera.yaw);
        float right_z   = -std::sin(main_camera.yaw);

        // Move Forward / Backward (W / S)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            main_camera.x += forward_x * move_speed;
            main_camera.z += forward_z * move_speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            main_camera.x -= forward_x * move_speed;
            main_camera.z -= forward_z * move_speed;
        }

        // Strafe Left / Right (A / D)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            main_camera.x -= right_x * move_speed;
            main_camera.z -= right_z * move_speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            main_camera.x += right_x * move_speed;
            main_camera.z += right_z * move_speed;
        }

        // Ascend / Descend vertically (Space / LShift)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))  main_camera.y += move_speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) main_camera.y -= move_speed;

        // =====================================================================
        // 3. ENGINE FRAME PASS
        // =====================================================================
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        game_tick(actor_store);
        submit_actors(actor_store, my_renderer);

        window.clear(sf::Color::Black);
        my_renderer.render(window, main_camera);
        window.display();
    }

    return 0;
}