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

    // =========================================================================
    // INLINE GRAPHICS UTILITIES
    // =========================================================================
    
    std::pair<float, float> viewport_to_canvas(float x, float y) {
        float cW = 100.0f;
        float cH = 100.0f;
        
        // Add 250.0f so (0,0) in 3D maps directly to the center of your 500x500 window
        return { (x * cW) + 250.0f, (y * cH) + 250.0f };
    }

    std::pair<float, float> project_vertex(const std::vector<float>& v) {
        if (v[2] <= 0.0f) return { 0.0f, 0.0f }; 
        return viewport_to_canvas(v[0] / v[2], v[1] / v[2]);
    }

    void draw_line(sf::RenderWindow& window, std::pair<float, float> point_a, std::pair<float, float> point_b) {
        sf::Vector2f pointA(point_a.first, point_a.second);
        sf::Vector2f pointB(point_b.first, point_b.second);

        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = pointA;
        line[0].color = sf::Color::White;
        line[1].position = pointB;

        window.draw(line);
    }

    void render_vertexes(sf::RenderWindow& window,std::pair<  std::vector<std::vector<float>> , std::vector<std::vector<int>>    >& vertexes_triangle_pair){
        auto vertexes_to_draw = vertexes_triangle_pair.first;
        auto m_triangles = vertexes_triangle_pair.second;
        std::vector<std::pair<float, float>> projected_cache(vertexes_to_draw.size());
        for (int i{};i < vertexes_to_draw.size();++i){
            projected_cache[i] = project_vertex(vertexes_to_draw[i]);
        }
        for (const auto& triangle : m_triangles) {
            if (triangle.size() < 3) continue; 

            draw_line(window, projected_cache[triangle[0]], projected_cache[triangle[1]]);
            draw_line(window, projected_cache[triangle[1]], projected_cache[triangle[2]]);
            draw_line(window, projected_cache[triangle[0]], projected_cache[triangle[2]]);
        }
    }

}