#pragma once
#include "item_loader.hpp"

class character{

    public:

        item_loader::mesh_actor m_limb1;
        item_loader::mesh_actor m_limb2;
    	item_loader::mesh_actor m_limb3;

        using GridPair = std::pair<std::vector<std::vector<float>>, std::vector<std::vector<int>>>;
        character(float x_offset, float y_offset, float z_offset) : 
		m_limb1{x_offset,y_offset, z_offset}, 
		m_limb2{x_offset, y_offset, z_offset}, 
		m_limb3{x_offset, y_offset, z_offset}
		{
            // define each part of the character, hardcoded  file loads
            // item_loader::mesh_actor limb_1(x_offset,y_offset,z_offset);
            m_limb1.load_obj_from_disk("assets/kirby_parts/ball.obj");

            // item_loader::mesh_actor limb_2(x_offset,y_offset,z_offset);
            m_limb2.load_obj_from_disk("assets/kirby_parts/kirby_foot.obj");
            
	        m_limb3.load_obj_from_disk("assets/kirby_parts/kirby_foot.obj");

            // set the parents offset to the child
            m_limb1.set_relative_position_to_parent(0,0,0);
            m_limb2.set_relative_position_to_parent(-1.0f,1.0f,0);
            m_limb3.set_relative_position_to_parent(1.0f,1.0f,0);
        }

        std::vector<GridPair> draw_self(){
            auto ret_1 = m_limb1.draw_self();
            auto ret_2 = m_limb2.draw_self();
    	    auto ret_3 = m_limb3.draw_self();
            return std::vector<GridPair> {ret_1,ret_2,ret_3};
        }

        void tick(){
            // TODO: We must pass into TICK some relative offsets, so any rotations are applied 
            // relative to an origin point
            m_limb1.tick();
            m_limb2.tick();
	        m_limb3.tick();
        }

};
