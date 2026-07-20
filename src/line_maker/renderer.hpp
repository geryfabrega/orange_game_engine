#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>

namespace renderer {

    struct Camera {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;
    };

    struct DirectionalLight {
        float dir_x = 0.0f;
        float dir_y = -1.0f;
        float dir_z = 1.0f;
        float ambient = 0.25f;
    };

    struct RenderPayload {
        std::vector<std::vector<float>> vertices;
        std::vector<std::vector<int>> triangles;
        sf::Color base_color = sf::Color::Cyan;
    };

    // Helper structure to bundle a transformed triangle with its depth and shaded color
    struct ProcessedTriangle {
        std::pair<float, float> p0;
        std::pair<float, float> p1;
        std::pair<float, float> p2;
        float avg_z;
        sf::Color color;
    };

    class renderer {
    private:
        std::vector<RenderPayload> m_submit_queue;

        std::pair<float, float> viewport_to_canvas(float x, float y) {
            float cW = 100.0f;
            float cH = 100.0f;
            return { (x * cW) + 250.0f, (y * cH) + 250.0f };
        }

        std::vector<float> apply_camera_transform(const std::vector<float>& vertex, const Camera& cam) {
            float tx = vertex[0] - cam.x;
            float ty = vertex[1] - cam.y;
            float tz = vertex[2] - cam.z;

            float cos_yaw = std::cos(-cam.yaw);
            float sin_yaw = std::sin(-cam.yaw);
            float cos_pitch = std::cos(-cam.pitch);
            float sin_pitch = std::sin(-cam.pitch);

            float rx = tx * cos_yaw - tz * sin_yaw;
            float ry = ty;
            float rz = tx * sin_yaw + tz * cos_yaw;

            float final_x = rx;
            float final_y = ry * cos_pitch - rz * sin_pitch;
            float final_z = ry * sin_pitch + rz * cos_pitch;

            return { final_x, final_y, final_z };
        }

        std::pair<float, float> project_vertex(const std::vector<float>& v) {
            if (v[2] <= 0.1f) return { -9999.0f, -9999.0f }; 
            return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
        }

        sf::Color calculate_lighting(const std::vector<float>& v0, 
                                   const std::vector<float>& v1, 
                                   const std::vector<float>& v2, 
                                   const sf::Color& base_color,
                                   const DirectionalLight& light) {
            float ax = v1[0] - v0[0], ay = v1[1] - v0[1], az = v1[2] - v0[2];
            float bx = v2[0] - v0[0], by = v2[1] - v0[1], bz = v2[2] - v0[2];

            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;

            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len == 0.0f) len = 1.0f;
            nx /= len; ny /= len; nz /= len;

            float lx = -light.dir_x, ly = -light.dir_y, lz = -light.dir_z;
            float llen = std::sqrt(lx * lx + ly * ly + lz * lz);
            if (llen == 0.0f) llen = 1.0f;
            lx /= llen; ly /= llen; lz /= llen;

            float dot = nx * lx + ny * ly + nz * lz;
            float intensity = std::max(light.ambient, std::min(1.0f, dot + light.ambient));

            return sf::Color(
                static_cast<sf::Uint8>(base_color.r * intensity),
                static_cast<sf::Uint8>(base_color.g * intensity),
                static_cast<sf::Uint8>(base_color.b * intensity)
            );
        }

        void draw_solid_triangle(sf::RenderWindow& window, const ProcessedTriangle& tri) {
            // Skip clipping sentinel points
            if (tri.p0.first == -9999.0f || tri.p1.first == -9999.0f || tri.p2.first == -9999.0f) return;

            sf::VertexArray va(sf::PrimitiveType::Triangles, 3);
            va[0].position = sf::Vector2f(tri.p0.first, tri.p0.second);
            va[1].position = sf::Vector2f(tri.p1.first, tri.p1.second);
            va[2].position = sf::Vector2f(tri.p2.first, tri.p2.second);

            va[0].color = tri.color;
            va[1].color = tri.color;
            va[2].color = tri.color;

            window.draw(va);
        }

    public:
        renderer() = default;

        void submit(const RenderPayload& payload) {
            m_submit_queue.push_back(payload);
        }

        void submit(RenderPayload&& payload) {
            m_submit_queue.push_back(std::move(payload));
        }

        void render(sf::RenderWindow& window, const Camera& main_camera, const DirectionalLight& light) {
            std::vector<ProcessedTriangle> raster_queue;

            // 1. Transform, Shade, and Calculate Depth for ALL triangles across ALL objects
            for (const auto& payload : m_submit_queue) {
                const auto& vertices = payload.vertices;
                const auto& triangles = payload.triangles;

                // Cache camera-space coordinates for accurate Z-depth calculation
                std::vector<std::vector<float>> view_vertices(vertices.size());
                std::vector<std::pair<float, float>> projected_cache(vertices.size());

                for (size_t i = 0; i < vertices.size(); ++i) {
                    view_vertices[i] = apply_camera_transform(vertices[i], main_camera);
                    projected_cache[i] = project_vertex(view_vertices[i]);
                }

                for (const auto& tri : triangles) {
                    if (tri.size() < 3) continue;

                    // Camera space Z values for depth calculation
                    float z0 = view_vertices[tri[0]][2];
                    float z1 = view_vertices[tri[1]][2];
                    float z2 = view_vertices[tri[2]][2];

                    // Back-face Culling: If any point is behind camera, don't queue
                    if (z0 <= 0.1f || z1 <= 0.1f || z2 <= 0.1f) continue;

                    float avg_z = (z0 + z1 + z2) / 3.0f;

                    sf::Color shaded_color = calculate_lighting(
                        vertices[tri[0]], vertices[tri[1]], vertices[tri[2]],
                        payload.base_color, light
                    );

                    raster_queue.push_back({
                        projected_cache[tri[0]],
                        projected_cache[tri[1]],
                        projected_cache[tri[2]],
                        avg_z,
                        shaded_color
                    });
                }
            }

            // 2. PAINTER'S ALGORITHM: Sort from furthest (highest Z) to closest (lowest Z)
            std::sort(raster_queue.begin(), raster_queue.end(), [](const ProcessedTriangle& a, const ProcessedTriangle& b) {
                return a.avg_z > b.avg_z;
            });

            // 3. Draw in back-to-front order
            for (const auto& tri : raster_queue) {
                draw_solid_triangle(window, tri);
            }

            m_submit_queue.clear();
        }
    };

}