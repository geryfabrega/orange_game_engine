#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <cmath>

namespace item_loader {

    // =========================================================================
    // INLINE GRAPHICS UTILITIES
    // =========================================================================
    
    inline std::pair<float, float> viewport_to_canvas(float x, float y) {
        float cW = 100.0f;
        float cH = 100.0f;
        
        // Add 250.0f so (0,0) in 3D maps directly to the center of your 500x500 window
        return { (x * cW) + 250.0f, (y * cH) + 250.0f };
    }

    inline std::pair<float, float> project_vertex(const std::vector<float>& v) {
        if (v[2] <= 0.0f) return { 0.0f, 0.0f }; 
        return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
    }

    inline void draw_line(sf::RenderWindow& window, std::pair<float, float> point_a, std::pair<float, float> point_b) {
        sf::Vector2f pointA(point_a.first, point_a.second);
        sf::Vector2f pointB(point_b.first, point_b.second);

        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = pointA;
        line[0].color = sf::Color::White;
        line[1].position = pointB;

        window.draw(line);
    }

    // =========================================================================
    // MESH ACTOR OBJECT DEFINITION
    // =========================================================================

    class mesh_actor {
    public:
        mesh_actor(const std::string& file_path, float x_offset, float y_offset, float z_offset)
            : m_x_offset(x_offset), m_y_offset(y_offset), m_z_offset(z_offset), m_angle(0.0f) {
            
            std::ifstream fs(file_path);
            if (!fs.is_open()) {
                std::cerr << "Error: Could not properly load OBJ file: " << file_path << std::endl;
                return;
            }

            std::string buffer;
            std::string inner_buffer;

            while (std::getline(fs, buffer)) {
                if (buffer.empty()) continue;

                if (buffer[0] == 'v') {
                    buffer.erase(0, 2); // Strip prefix 'v '
                    std::stringstream ss(buffer);
                    std::vector<float> coords;
                    
                    while (std::getline(ss, inner_buffer, ' ')) {
                        if (!inner_buffer.empty()) {
                            coords.push_back(std::stof(inner_buffer));
                        }
                    }
                    m_local_vertices.push_back(coords);

                } else if (buffer[0] == 'f') {
                    buffer.erase(0, 2); // Strip prefix 'f '
                    std::stringstream ss(buffer);
                    std::vector<int> tris;
                    
                    while (std::getline(ss, inner_buffer, ' ')) {
                        if (!inner_buffer.empty()) {
                            // Convert OBJ 1-based index to C++ 0-based index
                            tris.push_back(std::stoi(inner_buffer) - 1);
                        }
                    }
                    m_triangles.push_back(tris);
                }
            }
            std::cout << "Cached header-mesh. Vertices: " << m_local_vertices.size() 
                      << " | Faces: " << m_triangles.size() << std::endl;
        }

        void tick() {
            m_angle += 0.02f; // Increments animation speed
            if (m_angle > 6.28318f) {
                m_angle -= 6.28318f;
            }
        }

        std::pair<  std::vector<std::vector<float>> , std::vector<std::vector<int>>    > draw_self(){
            float cos_a = std::cos(m_angle);
            float sin_a = std::sin(m_angle);

            // Transform local vertices into world coordinates using stack cache allocation
            // std::vector<std::pair<float, float>> projected_cache(m_local_vertices.size());
            std::vector<std::vector<float>> mutated_world_vertexes(m_local_vertices.size());

            for (size_t i = 0; i < m_local_vertices.size(); ++i) {
                float x = m_local_vertices[i][0];
                float y = m_local_vertices[i][1];
                float z = m_local_vertices[i][2];

                // Y-axis Rotation calculations
                float rot_x = x * cos_a - z * sin_a;
                float rot_y = y;
                float rot_z = x * sin_a + z * cos_a;

                std::vector<float> transformed_vertex = {
                    rot_x + m_x_offset,
                    rot_y + m_y_offset,
                    rot_z + m_z_offset
                };

                mutated_world_vertexes[i] = transformed_vertex;
            }
            std::pair<  std::vector<std::vector<float>> , std::vector<std::vector<int>>    > ret_pair{mutated_world_vertexes,m_triangles};
            return ret_pair;
        }

    private:
        std::vector<std::vector<float>> m_local_vertices;
        std::vector<std::vector<int>> m_triangles;

        float m_x_offset;
        float m_y_offset;
        float m_z_offset;
        float m_angle;
    };

} // namespace item_loader