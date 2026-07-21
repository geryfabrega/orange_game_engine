#pragma once
#include "item_loader.hpp"

class character : item_loader::mesh_actor{
    character() : item_loader::mesh_actor(0.0f,0.0f,0.0f){
        // define each part of the character, hardcoded  file loads
        load_obj_from_disk("assets/cow.obj",0.0f,0.0f,0.0f);
    }
};