#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <memory>
#include "line_maker/line_maker.hh"
#include "line_maker/item_loader.hpp"
#include "line_maker/renderer.hpp"
#include "line_maker/load_character.hpp"
#include "line_maker/terminal_shell.hpp"

void game_tick(std::vector<std::unique_ptr<character>>& actor_vec) {
    physics::update_world(actor_vec);
}

void submit_actors(std::vector<std::unique_ptr<character>>& actor_vec, renderer::renderer& render_obj) {
    for (auto& act : actor_vec) {
        // 1. Submit Polygon Meshes for all limbs
        for (auto& payload : act->draw_self()) {
            render_obj.submit(std::move(payload));
        }

        // 2. Submit Textures (Decals & Sky Clouds)
        for (auto& tex_payload : act->draw_textures()) {
            render_obj.submit_texture(tex_payload);
        }
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "SFML 3D Platformer + In-Game Linux Terminal");
    
    renderer::renderer my_renderer;
    my_renderer.set_ps1_effect(true, 2.0f); // Higher PS1 vertex snapping jitter (4.0px grid)

    // 3D Platformer Follow Camera
    renderer::Camera main_camera;
    main_camera.yaw = 0.0f;
    main_camera.pitch = 0.28f; // Angled downward to view character and platform

    float cam_dist = 7.0f;
    const float cam_height_offset = 1.2f;
    const float zoom_speed = 0.25f;

    const int target_fps = 60;
    const std::chrono::nanoseconds frame_duration(1000000000 / target_fps);
    auto next_frame_time = std::chrono::steady_clock::now();

    // Directional Sunlight (Warm bright daylight shining from sky onto ground)
    renderer::DirectionalLight sun_light;
    sun_light.dir_x = -0.3f;
    sun_light.dir_y =  1.0f;
    sun_light.dir_z =  0.5f;
    sun_light.ambient = 0.55f;

    // Character Collection: Includes flat grass platform, player, computer terminal, and sky clouds
    std::vector<std::unique_ptr<character>> actor_store;

    // 1. Flat 3D Grass Ground Platform (Lush vibrant grass green)
    auto ground_floor = std::make_unique<ground_actor>(0.0f, 0.0f, 0.0f, sf::Color(105, 215, 85));

    auto island_1 = std::make_unique<island_actor>(0.0f, 0.0f, 0.0f);
    actor_store.push_back(std::move(island_1));
    actor_store.push_back(std::move(ground_floor));

    // 2. Interactive Retro Computer Terminal in the 3D World
    auto comp_terminal = std::make_unique<terminal_actor>(4.0f, 0.0f, 4.0f);
    terminal_actor* terminal_actor_ptr = comp_terminal.get();
    actor_store.push_back(std::move(comp_terminal));

    // 3. The Player Character Actor (Kirby with Hitbox, Jumping & Gravity)
    auto player_actor = std::make_unique<kirby_character>(0.0f, 0.0f, 0.0f);
    kirby_character* player_ptr = player_actor.get();
    actor_store.push_back(std::move(player_actor));

    // 4. Drifting Fluffy Sky Clouds
    actor_store.push_back(std::make_unique<cloud_actor>(-25.0f, -12.0f, 35.0f, 11.0f, 5.5f, 0.015f));
    actor_store.push_back(std::make_unique<cloud_actor>( -5.0f, -15.0f, 45.0f, 15.0f, 7.5f, 0.020f));
    actor_store.push_back(std::make_unique<cloud_actor>( 20.0f, -13.0f, 30.0f, 10.0f, 5.0f, 0.018f));
    actor_store.push_back(std::make_unique<cloud_actor>( 35.0f, -16.0f, 40.0f, 13.0f, 6.5f, 0.012f));

    // In-Game Linux Bash Shell Terminal System
    in_game_terminal terminal_shell_ui;

    const float rot_speed = 0.035f;
    const float char_move_speed = 0.14f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // Pass input events to terminal shell when active
            if (terminal_shell_ui.is_open) {
                terminal_shell_ui.handle_event(event);
                continue;
            }

            // Mouse wheel camera zoom
            if (event.type == sf::Event::MouseWheelScrolled) {
                cam_dist -= event.mouseWheelScroll.delta * 0.8f;
                cam_dist = std::max(2.5f, std::min(22.0f, cam_dist));
            }
            
            // Jump on Space key press
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                if (player_ptr) {
                    player_ptr->jump();
                }
            }

            // Terminal Interaction Trigger: Press X when near the computer terminal
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::X) {
                if (player_ptr && terminal_actor_ptr) {
                    float dx = player_ptr->m_x_offset - terminal_actor_ptr->m_x_offset;
                    float dz = player_ptr->m_z_offset - terminal_actor_ptr->m_z_offset;
                    float dist = std::sqrt(dx * dx + dz * dz);
                    if (dist < 3.5f) {
                        terminal_shell_ui.open();
                    }
                }
            }

            // Adjust PS1 quantization step size dynamically with [ and ]
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::RBracket) {
                    my_renderer.ps1_step_size = std::min(10.0f, my_renderer.ps1_step_size + 1.0f);
                } else if (event.key.code == sf::Keyboard::LBracket) {
                    my_renderer.ps1_step_size = std::max(0.5f, my_renderer.ps1_step_size - 1.0f);
                }
            }
        }

        // When terminal is open, pause 3D player movement & camera rotation
        if (!terminal_shell_ui.is_open) {
            // =====================================================================
            // 1. CAMERA ZOOM CONTROLS (E to Zoom In, Q to Zoom Out, or +/-)
            // =====================================================================
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
                cam_dist = std::max(2.5f, cam_dist - zoom_speed);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) {
                cam_dist = std::min(22.0f, cam_dist + zoom_speed);
            }

            // Continuous Space check for responsive jumping
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                if (player_ptr && player_ptr->m_is_grounded) {
                    player_ptr->jump();
                }
            }

            // =====================================================================
            // 2. CAMERA ORBIT CONTROLS (Arrow Keys orbit around character)
            // =====================================================================
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  main_camera.yaw -= rot_speed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) main_camera.yaw += rot_speed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    main_camera.pitch -= rot_speed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  main_camera.pitch += rot_speed;

            // Clamp camera pitch
            const float min_pitch = -0.2f;
            const float max_pitch =  1.3f;
            if (main_camera.pitch > max_pitch) main_camera.pitch = max_pitch;
            if (main_camera.pitch < min_pitch) main_camera.pitch = min_pitch;

            // =====================================================================
            // 3. CHARACTER MOVEMENT ON FLAT 3D PLATFORM (WASD relative to Camera)
            // =====================================================================
            if (player_ptr) {
                float forward_x =  std::sin(main_camera.yaw);
                float forward_z =  std::cos(main_camera.yaw);
                float right_x   =  std::cos(main_camera.yaw);
                float right_z   = -std::sin(main_camera.yaw);

                float input_x = 0.0f;
                float input_z = 0.0f;

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                    input_x += forward_x;
                    input_z += forward_z;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                    input_x -= forward_x;
                    input_z -= forward_z;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                    input_x -= right_x;
                    input_z -= right_z;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                    input_x += right_x;
                    input_z += right_z;
                }

                // Normalize diagonal movement
                float input_len = std::sqrt(input_x * input_x + input_z * input_z);
                if (input_len > 0.0001f) {
                    input_x /= input_len;
                    input_z /= input_len;
                }

                // Move player character
                player_ptr->move(input_x * char_move_speed, input_z * char_move_speed);

                // =================================================================
                // 4. 3D PLATFORMER FOLLOW CAMERA (with Zoom distance)
                // =================================================================
                float cos_p = std::cos(main_camera.pitch);
                float sin_p = std::sin(main_camera.pitch);
                float cos_y = std::cos(main_camera.yaw);
                float sin_y = std::sin(main_camera.yaw);

                main_camera.x = player_ptr->m_x_offset - cam_dist * sin_y * cos_p;
                main_camera.z = player_ptr->m_z_offset - cam_dist * cos_y * cos_p;
                main_camera.y = player_ptr->m_y_offset - cam_dist * sin_p - cam_height_offset;
            }
        }

        // =====================================================================
        // 6. FRAME PASS: TICK, SUBMIT & UNIFIED RENDER
        // =====================================================================
        game_tick(actor_store);
        submit_actors(actor_store, my_renderer);

        // Bright Sky Blue Background
        window.clear(sf::Color(105, 190, 255));

        // Unified 3D Render Pass (Polygons, Kirby, Decals & Clouds in True Painter's Depth Order)
        my_renderer.render_scene(window, main_camera, sun_light);

        // =====================================================================
        // 7. 2D HUD / INTERACTION PROMPT & RETRO TERMINAL SHELL OVERLAY
        // =====================================================================
        if (!terminal_shell_ui.is_open && player_ptr && terminal_actor_ptr) {
            float dx = player_ptr->m_x_offset - terminal_actor_ptr->m_x_offset;
            float dz = player_ptr->m_z_offset - terminal_actor_ptr->m_z_offset;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist < 3.5f && terminal_shell_ui.has_font) {
                // Floating interaction banner
                sf::RectangleShape prompt_bg(sf::Vector2f(340.0f, 40.0f));
                prompt_bg.setPosition(205.0f, 660.0f);
                prompt_bg.setFillColor(sf::Color(10, 25, 15, 230));
                prompt_bg.setOutlineThickness(2.0f);
                prompt_bg.setOutlineColor(sf::Color(50, 255, 100));
                window.draw(prompt_bg);

                sf::Text prompt_text("[PRESS X TO ACCESS BASH TERMINAL]", terminal_shell_ui.font, 14);
                prompt_text.setPosition(215.0f, 670.0f);
                prompt_text.setFillColor(sf::Color(80, 255, 120));
                window.draw(prompt_text);
            }
        }

        // Draw In-Game Linux Terminal Shell if open
        if (terminal_shell_ui.is_open) {
            terminal_shell_ui.draw(window);
        }

        next_frame_time += frame_duration;
        std::this_thread::sleep_until(next_frame_time);
        window.display();
    }

    return 0;
}
