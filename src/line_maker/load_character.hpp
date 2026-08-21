#pragma once
#include "item_loader.hpp"

class character{

    public:
        using GridPair = std::pair<std::vector<std::vector<float>>, std::vector<std::vector<int>>>;
        character(float x_offset, float y_offset, float z_offset) : m_limb1{x_offset,y_offset, z_offset}, m_limb2{x_offset, y_offset + 5.0f, z_offset} {
            // define each part of the character, hardcoded  file loads
            // item_loader::mesh_actor limb_1(x_offset,y_offset,z_offset);
            m_limb1.load_obj_from_disk("assets/cow.obj");

            // item_loader::mesh_actor limb_2(x_offset,y_offset,z_offset);
            m_limb2.load_obj_from_disk("assets/teapot.obj");
        }

        std::vector<GridPair> draw_self(){
            auto ret_1 = m_limb1.draw_self();
            auto ret_2 = m_limb2.draw_self();
            // combine both vectors
            // ret_1.first.insert(ret_1.first.end(),ret_2.first.begin(),ret_2.first.end());
            // ret_1.second.insert(ret_1.second.end(),ret_2.second.begin(),ret_2.second.end());
            return std::vector<GridPair> {ret_1,ret_2};
        }
        item_loader::mesh_actor m_limb1;
        item_loader::mesh_actor m_limb2;

        void tick(){
            m_limb1.tick();
            m_limb2.tick();
        }

};