// line_maker.cc
#include "line_maker.hh"
#include <iostream>
#include <cmath> // Required for std::sin and std::cos

namespace linemaker {

    // =========================================================================
    // ANONYMOUS NAMESPACE (Hidden Implementation Details)
    // =========================================================================
    namespace {
        
        std::pair<float, float> viewport_to_canvas(float x, float y) {
            float cW = 100.0f;
            float cH = 100.0f;
            float vW = 1.0f;
            float vH = 1.0f;
            return { x * cW / vW, y * cH / vH };
        }

        std::pair<float, float> project_vertex(const std::vector<float>& v) {
            if (v[2] <= 0.0f) return { 0.0f, 0.0f }; 
            return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
        }

    } // anonymous namespace

    // =========================================================================
    // CUBE ACTOR MEMBER DEFINITIONS
    // =========================================================================

    cube_actor::cube_actor(float x_offset, float y_offset, float z_offset) 
        : m_x_offset(x_offset), m_y_offset(y_offset), m_z_offset(z_offset), m_angle(0.0f) {
        std::cout << "Cube init with offsets and compile-time scale: " << CUBE_SCALE << std::endl;
    }

    void cube_actor::tick() {
        // Increment the angle. Adjust 0.03f to speed up or slow down the spin.
        m_angle += 0.03f;
        
        // Wrap around 2*PI to prevent floating point overflow over time
        if (m_angle > 6.28318f) {
            m_angle -= 6.28318f;
        }
    }

    void cube_actor::draw_self(sf::RenderWindow& window) {
        // 1. Define pristine, unshifted Local/Model Space coordinates centered at origin (0,0,0)
        // These are then multiplied by our compile-time CUBE_SCALE variable.
        std::vector<std::vector<float>> local_vertices = {
            { -0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE }, // 0: front-bottom-left
            { -0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE }, // 1: front-top-left
            {  0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE }, // 2: front-top-right
            {  0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE }, // 3: front-bottom-right
            { -0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE }, // 4: back-bottom-left
            { -0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE }, // 5: back-top-left
            {  0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE }, // 6: back-top-right
            {  0.5f * CUBE_SCALE, -0.5f * CUBE_SCALE,  0.5f * CUBE_SCALE }  // 7: back-bottom-right
        };

        // Pre-calculate trig waves for this frame's rotation tick
        float cos_a = std::cos(m_angle);
        float sin_a = std::sin(m_angle);

        // 2. Transform vertices to World Space (Rotate then Translate)
        std::vector<std::vector<float>> transformed_vertices(8, std::vector<float>(3));
        
        for (size_t i = 0; i < local_vertices.size(); ++i) {
            float x = local_vertices[i][0];
            float y = local_vertices[i][1];
            float z = local_vertices[i][2];

            // Apply Y-Axis Rotation matrix:
            // X' = X * cos(A) - Z * sin(A)
            // Z' = X * sin(A) + Z * cos(A)
            float rot_x = x * cos_a - z * sin_a;
            float rot_y = y; // Y stays unchanged rotating around Y axis
            float rot_z = x * sin_a + z * cos_a;

            // Apply translation offsets (Z base matches original depth offset)
            transformed_vertices[i][0] = rot_x + m_x_offset;
            transformed_vertices[i][1] = rot_y + m_y_offset;
            transformed_vertices[i][2] = rot_z + m_z_offset + 5.5f; // Center depth around ~5.5
        }

        // 3. Render Wireframe using newly calculated indices mapping
        // Front face
        draw_line(window, project_vertex(transformed_vertices[0]), project_vertex(transformed_vertices[1]));
        draw_line(window, project_vertex(transformed_vertices[1]), project_vertex(transformed_vertices[2]));
        draw_line(window, project_vertex(transformed_vertices[2]), project_vertex(transformed_vertices[3]));
        draw_line(window, project_vertex(transformed_vertices[3]), project_vertex(transformed_vertices[0]));

        // Back face
        draw_line(window, project_vertex(transformed_vertices[4]), project_vertex(transformed_vertices[5]));
        draw_line(window, project_vertex(transformed_vertices[5]), project_vertex(transformed_vertices[6]));
        draw_line(window, project_vertex(transformed_vertices[6]), project_vertex(transformed_vertices[7]));
        draw_line(window, project_vertex(transformed_vertices[7]), project_vertex(transformed_vertices[4]));

        // Connecting links
        draw_line(window, project_vertex(transformed_vertices[0]), project_vertex(transformed_vertices[4]));
        draw_line(window, project_vertex(transformed_vertices[1]), project_vertex(transformed_vertices[5]));
        draw_line(window, project_vertex(transformed_vertices[2]), project_vertex(transformed_vertices[6]));
        draw_line(window, project_vertex(transformed_vertices[3]), project_vertex(transformed_vertices[7]));
    }

    // =========================================================================
    // GLOBAL MULTIMEDIA UTILITIES
    // =========================================================================

    void draw_line(sf::RenderWindow& window, std::pair<float, float> point_a, std::pair<float, float> point_b) {
        sf::Vector2f pointA(point_a.first, point_a.second);
        sf::Vector2f pointB(point_b.first, point_b.second);

        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = pointA;
        line[0].color = sf::Color::Cyan;
        line[1].position = pointB;
        line[1].color = sf::Color::Magenta;

        window.draw(line);
    }

} // namespace linemaker