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
        float x_offset_relative_to_parent = 0;
        float y_offset_relative_to_parent = 0;
        float z_offset_relative_to_parent = 0;
        float m_angle_x = 0.0f; // Rotation around X-axis (Pitch)
        float m_angle_y = 0.0f; // Rotation around Y-axis (Yaw)
        float m_angle_z = 0.0f; // Rotation around Z-axis (Roll)
        float m_angle = 0.0f;   // Backward-compatible alias for Y-axis rotation (Yaw)
	    float m_angle_real = 0.0f;
        sf::Color m_color = sf::Color::White;

        mesh_actor(float x_offset = 0.0f, float y_offset = 0.0f, float z_offset = 0.0f, sf::Color color = sf::Color::White)
            : m_x_offset(x_offset), m_y_offset(y_offset), m_z_offset(z_offset), m_color(color),
              m_angle_x(0.0f), m_angle_y(0.0f), m_angle_z(0.0f), m_angle(0.0f), m_angle_real(0.0f) {
                std::cout << "Place Holder" << std:: endl;
            }
        // IF the limb is the MAIN limb aka torso, set to 0,0,0. else set to limb relative position to main
        // body part
        void set_relative_position_to_parent(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
            x_offset_relative_to_parent = x;
            y_offset_relative_to_parent = y;
            z_offset_relative_to_parent = z;
        }

        void set_position(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
            m_x_offset = x;
            m_y_offset = y;
            m_z_offset = z;
        }

        void set_angle(float angle = 0.0f) {
            m_angle = angle;
            m_angle_y = angle;
        }

        void set_angle_x(float angle_x = 0.0f) {
            m_angle_x = angle_x;
        }

        void set_angle_y(float angle_y = 0.0f) {
            m_angle_y = angle_y;
            m_angle = angle_y;
        }

        void set_angle_z(float angle_z = 0.0f) {
            m_angle_z = angle_z;
        }

        void set_rotation(float angle_x = 0.0f, float angle_y = 0.0f, float angle_z = 0.0f) {
            m_angle_x = angle_x;
            m_angle_y = angle_y;
            m_angle = angle_y;
            m_angle_z = angle_z;
        }

        void set_color(const sf::Color& color) {
            m_color = color;
        }

        sf::Color get_color() const {
            return m_color;
        }
            
        void load_obj_from_disk(const std::string& file_path){
            std::cout << "Loading object" << std::endl;
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

	double lockToStep(float value, float step_size) {
	    // Add a microscopic bias to protect against 1.2999999999999999 scenarios
	    double step_space = value / step_size;
	    
	    // Round to nearest integer step, then multiply back
	    return std::round(step_space) * step_size;
	}

        void tick() {
            // making this a no-op for now, ticks should be done at character level.
            // m_angle_real += 0.1f; // Increments animation speed
	        // m_angle = lockToStep(m_angle_real,.005);
            // if (m_angle > 6.28318f) {
            //     m_angle -= 6.28318f;
            // }
            ;
        }


        std::pair<std::vector<std::vector<float>>, std::vector<std::vector<int>>> draw_self() {
            float eff_angle_y = (m_angle_y != 0.0f) ? m_angle_y : m_angle;
            float cos_x = std::cos(m_angle_x);
            float sin_x = std::sin(m_angle_x);
            float cos_y = std::cos(eff_angle_y);
            float sin_y = std::sin(eff_angle_y);
            float cos_z = std::cos(m_angle_z);
            float sin_z = std::sin(m_angle_z);

            std::vector<std::vector<float>> mutated_world_vertexes(m_local_vertices.size());

            for (size_t i = 0; i < m_local_vertices.size(); ++i) {
                float x = m_local_vertices[i][0] + x_offset_relative_to_parent;
                float y = m_local_vertices[i][1] + y_offset_relative_to_parent;
                float z = m_local_vertices[i][2] + z_offset_relative_to_parent;

                // 1. X-axis Rotation (Pitch / Barrel Roll)
                float rx1 = x;
                float ry1 = y * cos_x - z * sin_x;
                float rz1 = y * sin_x + z * cos_x;

                // 2. Y-axis Rotation (Yaw)
                float rx2 = rx1 * cos_y + rz1 * sin_y;
                float ry2 = ry1;
                float rz2 = -rx1 * sin_y + rz1 * cos_y;

                // 3. Z-axis Rotation (Roll)
                float rx3 = rx2 * cos_z - ry2 * sin_z;
                float ry3 = rx2 * sin_z + ry2 * cos_z;
                float rz3 = rz2;

                std::vector<float> transformed_vertex = {
                    rx3 + m_x_offset,
                    ry3 + m_y_offset,
                    rz3 + m_z_offset
                };

                mutated_world_vertexes[i] = transformed_vertex;
            }
            return { mutated_world_vertexes, m_triangles };
        }

    private:
        std::vector<std::vector<float>> m_local_vertices;
        std::vector<std::vector<int>> m_triangles;

        float m_x_offset;
        float m_y_offset;
        float m_z_offset;
    };

} // namespace item_loader
