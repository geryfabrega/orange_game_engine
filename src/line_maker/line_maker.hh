// line_maker.hh
#ifndef LINE_MAKER_HH
#define LINE_MAKER_HH

#include <SFML/Graphics.hpp>
#include <utility>
#include <vector>

namespace linemaker {

    // Decide scale multiplier at compile time
    constexpr float CUBE_SCALE = 10.0f;

    void draw_line(sf::RenderWindow& window, std::pair<float, float> point_a, std::pair<float, float> point_b);

    class cube_actor {
    public:
        cube_actor(float x_offset, float y_offset, float z_offset);
        
        void tick(); // Increments the animation angle
        void draw_self(sf::RenderWindow& window);

    private:
        // Cache the offsets to apply dynamically during rendering
        float m_x_offset;
        float m_y_offset;
        float m_z_offset;

        // Tracks the animation state for the sine/cosine waves
        float m_angle; 
    };

} // namespace linemaker

#endif // LINE_MAKER_HH