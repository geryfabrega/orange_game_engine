#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include "line_maker/line_maker.hh"
#include "line_maker/item_loader.hpp"
#include "line_maker/renderer.hpp"

class actor{
    public:
    int m_start_pos_x;
    int m_start_pox_y;
    int m_curr_x;
    int m_curr_y;
    bool moving_right = true;
    bool moving_down = true;
    sf::CircleShape m_actor_shape;
    actor(int x,int y) : m_start_pos_x{x}, m_start_pox_y{y}, m_actor_shape{10.f}, m_curr_x{x}, m_curr_y{y}{
        std::cout << "actor created" << std::endl;
        m_actor_shape.setFillColor(sf::Color::Green);
        m_actor_shape.setPosition(m_start_pos_x,m_start_pox_y);
    }
    void tick() {
        int x_delta = 5;
        int y_delta = 5;
        if (moving_right){
            m_curr_x += x_delta;
        } else {
            m_curr_x -= x_delta;
        }
        if (moving_down){
            m_curr_y += y_delta;
        } else {
            m_curr_y -= y_delta;
        }
        if (m_curr_x >= 500){
            moving_right = false;
        }
        if (m_curr_x <=0 ){
            moving_right = true;
        }
        if (m_curr_y >= 500){
            moving_down = false;
        }
        if (m_curr_y <=0 ){
            moving_down = true;
        }
        m_actor_shape.setPosition(m_curr_x,m_curr_y);
    }
};

void frame_drawer(std::vector<actor>& actor_vec,sf::RenderWindow& window){
    for (auto const& act : actor_vec){
        window.draw(act.m_actor_shape);
    }
}

void game_tick(std::vector<actor>& actor_vec){
    for (auto& act : actor_vec){
        act.tick();
    }
}

int main()
{
    // SFML 2 uses direct parameters for VideoMode, not a brace-enclosed list
    sf::RenderWindow window(sf::VideoMode(500, 500), "SFML 2 works!");
    std::vector<actor> actor_store;
    item_loader::mesh_actor teapot("assets/deet.obj", 0.0f, 0.0f, 20.0f);
    while (window.isOpen())
    {
        sf::Event event; // Allocate event object locally 
        while (window.pollEvent(event)) // Pass it by reference
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        window.clear();
        game_tick(actor_store);
        frame_drawer(actor_store,window);
        auto vertexes = teapot.draw_self();
        renderer::render_vertexes(window,vertexes);
        teapot.tick();
        window.display();
    }
}

