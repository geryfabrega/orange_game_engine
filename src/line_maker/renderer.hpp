#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <cmath>

namespace renderer {

    // Camera representation
    struct Camera {
        float x = 0.0f, y = 0.0f, z = 0.0f; // Position
        float pitch = 0.0f;                 // Rotation looking up/down (radians)
        float yaw = 0.0f;                   // Rotation looking left/right (radians)
    };

    // Helper alias to keep the type signatures readable
    using MeshData = std::pair<std::vector<std::vector<float>>, std::vector<std::vector<int>>>;

    // =========================================================================
    // GRAPHICS UTILITIES
    // =========================================================================
    class renderer {
    private:
        // Holds all submitted mesh payloads until render() is called
        std::vector<MeshData> m_submit_queue;

        std::pair<float, float> viewport_to_canvas(float x, float y) {
            float cW = 100.0f;
            float cH = 100.0f;
            
            // Add 250.0f so (0,0) in 3D maps directly to the center of your 500x500 window
            return { (x * cW) + 250.0f, (y * cH) + 250.0f };
        }

        // Transforms World Space -> View (Camera) Space by applying OPPOSITE translation & rotation
        std::vector<float> apply_camera_transform(const std::vector<float>& vertex, const Camera& cam) {
            // 1. Opposite Translation (Move world relative to camera)
            float tx = vertex[0] - cam.x;
            float ty = vertex[1] - cam.y;
            float tz = vertex[2] - cam.z;

            // Pre-calculate trigonometric functions for speed
            float cos_yaw = std::cos(-cam.yaw);
            float sin_yaw = std::sin(-cam.yaw);
            float cos_pitch = std::cos(-cam.pitch);
            float sin_pitch = std::sin(-cam.pitch);

            // 2. Opposite Yaw (Y-axis rotation around camera)
            float rx = tx * cos_yaw - tz * sin_yaw;
            float ry = ty;
            float rz = tx * sin_yaw + tz * cos_yaw;

            // 3. Opposite Pitch (X-axis rotation around camera)
            float final_x = rx;
            float final_y = ry * cos_pitch - rz * sin_pitch;
            float final_z = ry * sin_pitch + rz * cos_pitch;

            return { final_x, final_y, final_z };
        }

        std::pair<float, float> project_vertex(const std::vector<float>& v) {
            // Near-plane clipping guard (don't draw stuff behind or directly on the camera)
            if (v[2] <= 0.1f) return { -9999.0f, -9999.0f }; 
            return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
        }

        void draw_line(sf::RenderWindow& window, std::pair<float, float> point_a, std::pair<float, float> point_b) {
            // Skip lines that fall outside valid projection space
            if (point_a.first == -9999.0f || point_b.first == -9999.0f) return;

            sf::Vector2f pointA(point_a.first, point_a.second);
            sf::Vector2f pointB(point_b.first, point_b.second);

            sf::VertexArray line(sf::PrimitiveType::Lines, 2);
            line[0].position = pointA;
            line[0].color = sf::Color::White;
            line[1].position = pointB;
            line[1].color = sf::Color::White;

            window.draw(line);
        }

        // Processes and draws a single mesh payload using the active camera
        void draw_mesh_payload(sf::RenderWindow& window, const MeshData& vertexes_triangle_pair, const Camera& cam) {
            const auto& vertexes_to_draw = vertexes_triangle_pair.first;
            const auto& m_triangles = vertexes_triangle_pair.second;

            std::vector<std::pair<float, float>> projected_cache(vertexes_to_draw.size());
            
            for (size_t i = 0; i < vertexes_to_draw.size(); ++i) {
                // Step A: Convert world coords to camera view coords
                std::vector<float> view_vertex = apply_camera_transform(vertexes_to_draw[i], cam);
                
                // Step B: Project view coords into 2D screen space
                projected_cache[i] = project_vertex(view_vertex);
            }

            for (const auto& triangle : m_triangles) {
                if (triangle.size() < 3) continue; 

                draw_line(window, projected_cache[triangle[0]], projected_cache[triangle[1]]);
                draw_line(window, projected_cache[triangle[1]], projected_cache[triangle[2]]);
                draw_line(window, projected_cache[triangle[0]], projected_cache[triangle[2]]);
            }
        }

    public:
        renderer() = default;

        void submit(const MeshData& mesh_data) {
            m_submit_queue.push_back(mesh_data);
        }

        void submit(MeshData&& mesh_data) {
            m_submit_queue.push_back(std::move(mesh_data));
        }

        // render() now accepts the Camera object to transform geometry relative to it
        void render(sf::RenderWindow& window, const Camera& main_camera) {
            for (const auto& payload : m_submit_queue) {
                draw_mesh_payload(window, payload, main_camera);
            }

            m_submit_queue.clear();
        }
    };

}