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
        sf::Color base_color = sf::Color::Blue;
    };

    struct TextureRenderPayload {
        const sf::Texture* texture = nullptr;
        float world_x = 0.0f;
        float world_y = 0.0f;
        float world_z = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
        float angle = 0.0f; // character / object rotation around Y-axis (radians)
        sf::Color tint = sf::Color::White;
        bool billboard = false;
        bool is_horizontal_plane = false; // If true, lies flat in horizontal XZ plane (for ground/floor)
        bool cull_backface = true;
    };

    // Helper structure to bundle a transformed triangle with its depth and shaded color
    struct ProcessedTriangle {
        std::pair<float, float> p0;
        std::pair<float, float> p1;
        std::pair<float, float> p2;
        float avg_z;
        sf::Color color;
    };

    // Helper structure for processed textured triangles during the texture rendering pass
    struct ProcessedTexTriangle {
        const sf::Texture* texture = nullptr;
        std::pair<float, float> p0, p1, p2;
        sf::Vector2f uv0, uv1, uv2;
        float avg_z;
        sf::Color tint;
    };

    struct ViewVertex {
        float x, y, z;
    };

    struct ViewTexVertex {
        float x, y, z;
        float u, v; // Texture coords (in pixels)
    };

    // Near-plane polygon clipping against Z = near_z
    inline std::vector<std::vector<ViewVertex>> clip_triangle_near(const ViewVertex& v0, const ViewVertex& v1, const ViewVertex& v2, float near_z = 0.1f) {
        std::vector<std::vector<ViewVertex>> result;
        const ViewVertex* verts[3] = { &v0, &v1, &v2 };
        bool inside[3] = { v0.z >= near_z, v1.z >= near_z, v2.z >= near_z };
        int inside_count = (inside[0] ? 1 : 0) + (inside[1] ? 1 : 0) + (inside[2] ? 1 : 0);

        if (inside_count == 0) {
            return result; // All behind near plane
        }

        if (inside_count == 3) {
            result.push_back({ v0, v1, v2 });
            return result;
        }

        if (inside_count == 1) {
            int i = inside[0] ? 0 : (inside[1] ? 1 : 2);
            int j = (i + 1) % 3;
            int k = (i + 2) % 3;

            const ViewVertex& A = *verts[i];
            const ViewVertex& B = *verts[j];
            const ViewVertex& C = *verts[k];

            float tAB = (near_z - A.z) / (B.z - A.z);
            float tAC = (near_z - A.z) / (C.z - A.z);

            ViewVertex pAB = { A.x + tAB * (B.x - A.x), A.y + tAB * (B.y - A.y), near_z };
            ViewVertex pAC = { A.x + tAC * (C.x - A.x), A.y + tAC * (C.y - A.y), near_z };

            result.push_back({ A, pAB, pAC });
            return result;
        }

        if (inside_count == 2) {
            int i = !inside[0] ? 0 : (!inside[1] ? 1 : 2);
            int j = (i + 1) % 3;
            int k = (i + 2) % 3;

            const ViewVertex& C = *verts[i]; // Outside vertex
            const ViewVertex& A = *verts[j]; // Inside vertex 1
            const ViewVertex& B = *verts[k]; // Inside vertex 2

            float tBC = (near_z - B.z) / (C.z - B.z);
            float tCA = (near_z - C.z) / (A.z - C.z);

            ViewVertex pBC = { B.x + tBC * (C.x - B.x), B.y + tBC * (C.y - B.y), near_z };
            ViewVertex pCA = { C.x + tCA * (A.x - C.x), C.y + tCA * (A.y - C.y), near_z };

            result.push_back({ A, B, pCA });
            result.push_back({ B, pBC, pCA });
            return result;
        }

        return result;
    }

    // Near-plane textured polygon clipping against Z = near_z
    inline std::vector<std::vector<ViewTexVertex>> clip_tex_triangle_near(const ViewTexVertex& v0, const ViewTexVertex& v1, const ViewTexVertex& v2, float near_z = 0.1f) {
        std::vector<std::vector<ViewTexVertex>> result;
        const ViewTexVertex* verts[3] = { &v0, &v1, &v2 };
        bool inside[3] = { v0.z >= near_z, v1.z >= near_z, v2.z >= near_z };
        int inside_count = (inside[0] ? 1 : 0) + (inside[1] ? 1 : 0) + (inside[2] ? 1 : 0);

        if (inside_count == 0) {
            return result; // All behind near plane
        }

        if (inside_count == 3) {
            result.push_back({ v0, v1, v2 });
            return result;
        }

        if (inside_count == 1) {
            int i = inside[0] ? 0 : (inside[1] ? 1 : 2);
            int j = (i + 1) % 3;
            int k = (i + 2) % 3;

            const ViewTexVertex& A = *verts[i];
            const ViewTexVertex& B = *verts[j];
            const ViewTexVertex& C = *verts[k];

            float tAB = (near_z - A.z) / (B.z - A.z);
            float tAC = (near_z - A.z) / (C.z - A.z);

            ViewTexVertex pAB = {
                A.x + tAB * (B.x - A.x),
                A.y + tAB * (B.y - A.y),
                near_z,
                A.u + tAB * (B.u - A.u),
                A.v + tAB * (B.v - A.v)
            };
            ViewTexVertex pAC = {
                A.x + tAC * (C.x - A.x),
                A.y + tAC * (C.y - A.y),
                near_z,
                A.u + tAC * (C.u - A.u),
                A.v + tAC * (C.v - A.v)
            };

            result.push_back({ A, pAB, pAC });
            return result;
        }

        if (inside_count == 2) {
            int i = !inside[0] ? 0 : (!inside[1] ? 1 : 2);
            int j = (i + 1) % 3;
            int k = (i + 2) % 3;

            const ViewTexVertex& C = *verts[i]; // Outside vertex
            const ViewTexVertex& A = *verts[j]; // Inside vertex 1
            const ViewTexVertex& B = *verts[k]; // Inside vertex 2

            float tBC = (near_z - B.z) / (C.z - B.z);
            float tCA = (near_z - C.z) / (A.z - C.z);

            ViewTexVertex pBC = {
                B.x + tBC * (C.x - B.x),
                B.y + tBC * (C.y - B.y),
                near_z,
                B.u + tBC * (C.u - B.u),
                B.v + tBC * (C.v - B.v)
            };
            ViewTexVertex pCA = {
                C.x + tCA * (A.x - C.x),
                C.y + tCA * (A.y - C.y),
                near_z,
                C.u + tCA * (A.u - C.u),
                C.v + tCA * (A.v - C.v)
            };

            result.push_back({ A, B, pCA });
            result.push_back({ B, pBC, pCA });
            return result;
        }

        return result;
    }

    class renderer {
    public:
        bool ps1_effect_enabled = true;
        float ps1_step_size = 2.0f; // Step size for PS1 vertex grid quantization

        // PS1 style vertex quantization / grid snapping
        double lockToStep(float value, float step_size) {
            // Add a microscopic bias to protect against 1.2999999999999999 scenarios
            double step_space = value / step_size;
            
            // Round to nearest integer step, then multiply back
            return std::round(step_space) * step_size;
        }

        std::pair<float, float> viewport_to_canvas(float x, float y) {
            float cW = 100.0f;
            float cH = 100.0f;
            float px = (x * cW) + 250.0f;
            float py = (y * cH) + 250.0f;

            if (ps1_effect_enabled && ps1_step_size > 0.0f) {
                px = static_cast<float>(lockToStep(px, ps1_step_size));
                py = static_cast<float>(lockToStep(py, ps1_step_size));
            }

            return { px, py };
        }

        std::pair<float, float> project_vertex(const std::vector<float>& v) {
            if (v[2] <= 0.001f) return { -9999.0f, -9999.0f }; 
            return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
        }

        void set_ps1_effect(bool enable, float step_size = 2.0f) {
            ps1_effect_enabled = enable;
            ps1_step_size = step_size;
        }

    private:
        std::vector<RenderPayload> m_submit_queue;
        std::vector<TextureRenderPayload> m_texture_submit_queue;

        std::vector<float> apply_camera_transform(const std::vector<float>& vertex, const Camera& cam) {
            float tx = vertex[0] - cam.x;
            float ty = vertex[1] - cam.y;
            float tz = vertex[2] - cam.z;

            float cos_yaw = std::cos(cam.yaw);
            float sin_yaw = std::sin(cam.yaw);
            float cos_pitch = std::cos(cam.pitch);
            float sin_pitch = std::sin(cam.pitch);

            float rx = tx * cos_yaw - tz * sin_yaw;
            float ry = ty;
            float rz = tx * sin_yaw + tz * cos_yaw;

            float final_x = rx;
            float final_y = ry * cos_pitch - rz * sin_pitch;
            float final_z = ry * sin_pitch + rz * cos_pitch;

            return { final_x, final_y, final_z };
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
            if (llen == 0.0f) len = 1.0f;
            lx /= llen; ly /= llen; lz /= llen;

            float dot = nx * lx + ny * ly + nz * lz;
            float intensity = std::max(light.ambient, std::min(1.0f, dot + light.ambient));

            return sf::Color(
                static_cast<sf::Uint8>(base_color.r * intensity),
                static_cast<sf::Uint8>(base_color.g * intensity),
                static_cast<sf::Uint8>(base_color.b * intensity)
            );
        }

        void draw_solid_triangle(sf::RenderTarget& window, const ProcessedTriangle& tri) {
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
        renderer() {
            raster_queue.reserve(10000);
            texture_raster_queue.reserve(100);
        };

        std::vector<ProcessedTriangle> raster_queue;
        std::vector<ProcessedTexTriangle> texture_raster_queue;

        // Polygon Submission
        void submit(const RenderPayload& payload) {
            m_submit_queue.push_back(payload);
        }

        void submit(RenderPayload&& payload) {
            m_submit_queue.push_back(std::move(payload));
        }

        // Texture Submission
        void submit_texture(const TextureRenderPayload& payload) {
            m_texture_submit_queue.push_back(payload);
        }

        void submit_texture(TextureRenderPayload&& payload) {
            m_texture_submit_queue.push_back(std::move(payload));
        }

        void submit(const TextureRenderPayload& payload) {
            m_texture_submit_queue.push_back(payload);
        }

        void submit(TextureRenderPayload&& payload) {
            m_texture_submit_queue.push_back(std::move(payload));
        }

        // =========================================================================
        // UNIFIED RENDERER: PAINTER'S ALGORITHM ACROSS ALL POLYGONS & TEXTURES
        // =========================================================================
        enum class RenderItemType {
            TRIANGLE,
            TEXTURE_TRIANGLE
        };

        struct UnifiedRenderItem {
            RenderItemType type;
            float avg_z;
            ProcessedTriangle tri;
            ProcessedTexTriangle tex_tri;
        };

        std::vector<UnifiedRenderItem> unified_queue;

        void render_scene(sf::RenderTarget& window, const Camera& main_camera, const DirectionalLight& light) {
            unified_queue.clear();

            // 1. Process all Polygon Triangles with Near-Plane Clipping
            for (const auto& payload : m_submit_queue) {
                const auto& vertices = payload.vertices;
                const auto& triangles = payload.triangles;

                std::vector<ViewVertex> view_vertices(vertices.size());
                for (size_t i = 0; i < vertices.size(); ++i) {
                    auto tv = apply_camera_transform(vertices[i], main_camera);
                    view_vertices[i] = { tv[0], tv[1], tv[2] };
                }

                for (const auto& tri : triangles) {
                    if (tri.size() < 3) continue;

                    const auto& v0 = view_vertices[tri[0]];
                    const auto& v1 = view_vertices[tri[1]];
                    const auto& v2 = view_vertices[tri[2]];

                    auto clipped_tris = clip_triangle_near(v0, v1, v2, 0.1f);
                    if (clipped_tris.empty()) continue;

                    sf::Color shaded_color = calculate_lighting(
                        vertices[tri[0]], vertices[tri[1]], vertices[tri[2]],
                        payload.base_color, light
                    );

                    for (const auto& ctri : clipped_tris) {
                        float avg_z = (ctri[0].z + ctri[1].z + ctri[2].z) / 3.0f;
                        auto p0 = project_vertex({ ctri[0].x, ctri[0].y, ctri[0].z });
                        auto p1 = project_vertex({ ctri[1].x, ctri[1].y, ctri[1].z });
                        auto p2 = project_vertex({ ctri[2].x, ctri[2].y, ctri[2].z });

                        if (p0.first == -9999.0f || p1.first == -9999.0f || p2.first == -9999.0f) continue;

                        ProcessedTriangle pt{
                            p0,
                            p1,
                            p2,
                            avg_z,
                            shaded_color
                        };

                        UnifiedRenderItem item;
                        item.type = RenderItemType::TRIANGLE;
                        item.avg_z = avg_z;
                        item.tri = pt;
                        unified_queue.push_back(item);
                    }
                }
            }

            // 2. Process all Textures (Decals, Ground Textures & Clouds) with Near-Plane Clipping
            for (const auto& payload : m_texture_submit_queue) {
                if (!payload.texture || payload.texture->getSize().x == 0 || payload.texture->getSize().y == 0) {
                    continue;
                }

                // Backface Culling for vertical decals
                if (payload.cull_backface && !payload.billboard && !payload.is_horizontal_plane) {
                    float nx = std::sin(payload.angle);
                    float nz = std::cos(payload.angle);

                    float to_cam_x = main_camera.x - payload.world_x;
                    float to_cam_z = main_camera.z - payload.world_z;

                    float dot = nx * to_cam_x + nz * to_cam_z;
                    if (dot <= 0.0f) {
                        continue;
                    }
                }

                float hw = payload.width * 0.5f;
                float hh = payload.height * 0.5f;

                std::vector<float> tl, tr, br, bl;

                if (payload.is_horizontal_plane) {
                    float cos_a = std::cos(payload.angle);
                    float sin_a = std::sin(payload.angle);

                    tl = { payload.world_x - hw * cos_a - hh * sin_a, payload.world_y, payload.world_z + hw * sin_a - hh * cos_a };
                    tr = { payload.world_x + hw * cos_a - hh * sin_a, payload.world_y, payload.world_z - hw * sin_a - hh * cos_a };
                    br = { payload.world_x + hw * cos_a + hh * sin_a, payload.world_y, payload.world_z - hw * sin_a + hh * cos_a };
                    bl = { payload.world_x - hw * cos_a + hh * sin_a, payload.world_y, payload.world_z + hw * sin_a + hh * cos_a };
                } else {
                    float tx = payload.billboard ? std::cos(main_camera.yaw) : std::cos(payload.angle);
                    float tz = payload.billboard ? -std::sin(main_camera.yaw) : -std::sin(payload.angle);

                    tl = { payload.world_x - hw * tx, payload.world_y - hh, payload.world_z - hw * tz };
                    tr = { payload.world_x + hw * tx, payload.world_y - hh, payload.world_z + hw * tz };
                    br = { payload.world_x + hw * tx, payload.world_y + hh, payload.world_z + hw * tz };
                    bl = { payload.world_x - hw * tx, payload.world_y + hh, payload.world_z - hw * tz };
                }

                auto c0 = apply_camera_transform(tl, main_camera);
                auto c1 = apply_camera_transform(tr, main_camera);
                auto c2 = apply_camera_transform(br, main_camera);
                auto c3 = apply_camera_transform(bl, main_camera);

                sf::Vector2u tex_size = payload.texture->getSize();
                float tw = static_cast<float>(tex_size.x);
                float th = static_cast<float>(tex_size.y);

                ViewTexVertex v0 = { c0[0], c0[1], c0[2], 0.0f, 0.0f };
                ViewTexVertex v1 = { c1[0], c1[1], c1[2], tw, 0.0f };
                ViewTexVertex v2 = { c2[0], c2[1], c2[2], tw, th };
                ViewTexVertex v3 = { c3[0], c3[1], c3[2], 0.0f, th };

                // Split Quad into 2 Triangles: (v0, v1, v2) and (v0, v2, v3)
                std::vector<std::pair<ViewTexVertex, std::pair<ViewTexVertex, ViewTexVertex>>> quad_tris = {
                    { v0, { v1, v2 } },
                    { v0, { v2, v3 } }
                };

                for (const auto& qtri : quad_tris) {
                    auto clipped_tex_tris = clip_tex_triangle_near(qtri.first, qtri.second.first, qtri.second.second, 0.1f);
                    for (const auto& ctri : clipped_tex_tris) {
                        float avg_z = (ctri[0].z + ctri[1].z + ctri[2].z) / 3.0f;
                        auto p0 = project_vertex({ ctri[0].x, ctri[0].y, ctri[0].z });
                        auto p1 = project_vertex({ ctri[1].x, ctri[1].y, ctri[1].z });
                        auto p2 = project_vertex({ ctri[2].x, ctri[2].y, ctri[2].z });

                        if (p0.first == -9999.0f || p1.first == -9999.0f || p2.first == -9999.0f) continue;

                        ProcessedTexTriangle pttex{
                            payload.texture,
                            p0, p1, p2,
                            sf::Vector2f(ctri[0].u, ctri[0].v),
                            sf::Vector2f(ctri[1].u, ctri[1].v),
                            sf::Vector2f(ctri[2].u, ctri[2].v),
                            avg_z,
                            payload.tint
                        };

                        UnifiedRenderItem item;
                        item.type = RenderItemType::TEXTURE_TRIANGLE;
                        item.avg_z = avg_z;
                        item.tex_tri = pttex;
                        unified_queue.push_back(item);
                    }
                }
            }

            // 3. UNIFIED PAINTER'S ALGORITHM: Sort all triangles & textures back-to-front
            std::sort(unified_queue.begin(), unified_queue.end(), [](const UnifiedRenderItem& a, const UnifiedRenderItem& b) {
                return a.avg_z > b.avg_z;
            });

            // 4. Draw all items in strict back-to-front depth order
            for (const auto& item : unified_queue) {
                if (item.type == RenderItemType::TRIANGLE) {
                    draw_solid_triangle(window, item.tri);
                } else if (item.type == RenderItemType::TEXTURE_TRIANGLE) {
                    sf::VertexArray va(sf::PrimitiveType::Triangles, 3);

                    va[0].position = sf::Vector2f(item.tex_tri.p0.first, item.tex_tri.p0.second);
                    va[0].texCoords = item.tex_tri.uv0;
                    va[0].color = item.tex_tri.tint;

                    va[1].position = sf::Vector2f(item.tex_tri.p1.first, item.tex_tri.p1.second);
                    va[1].texCoords = item.tex_tri.uv1;
                    va[1].color = item.tex_tri.tint;

                    va[2].position = sf::Vector2f(item.tex_tri.p2.first, item.tex_tri.p2.second);
                    va[2].texCoords = item.tex_tri.uv2;
                    va[2].color = item.tex_tri.tint;

                    sf::RenderStates states;
                    states.texture = item.tex_tri.texture;
                    window.draw(va, states);
                }
            }

            m_submit_queue.clear();
            m_texture_submit_queue.clear();
        }

        void render(sf::RenderTarget& window, const Camera& main_camera, const DirectionalLight& light) {
            render_scene(window, main_camera, light);
        }

        void render_polygons(sf::RenderTarget& window, const Camera& main_camera, const DirectionalLight& light) {
            render_scene(window, main_camera, light);
        }

        void render_textures(sf::RenderTarget& window, const Camera& main_camera) {
            // Handled in unified render_scene
        }
    };

}