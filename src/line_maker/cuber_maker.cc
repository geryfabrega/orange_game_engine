#include "cuber_maker.hpp"
#include <SFML/Graphics.hpp>

void cubemaker::test_message(){ std::cout << "Test message working" << std::endl;}


void cubemaker::generate_box(){
    sf::RenderWindow window(sf::VideoMode({200, 200}), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);
}
