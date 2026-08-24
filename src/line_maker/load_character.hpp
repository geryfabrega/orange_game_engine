#pragma once
#include "item_loader.hpp"
#include "renderer.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <memory>
#include <algorithm>

// =========================================================================
// 3D AXIS-ALIGNED BOUNDING HITBOX & COLLISION
// =========================================================================
struct AABB {
    float min_x, max_x;
    float min_y, max_y;
    float min_z, max_z;

    bool overlaps(const AABB& other) const {
        return (min_x < other.max_x && max_x > other.min_x) &&
               (min_y < other.max_y && max_y > other.min_y) &&
               (min_z < other.max_z && max_z > other.min_z);
    }
};

struct Hitbox {
    float offset_x    = 0.0f;
    float offset_y    = 0.0f;
    float offset_z    = 0.0f;
    float half_width  = 0.9f; // X extents
    float half_height = 1.0f; // Y extents (vertical)
    float half_depth  = 0.9f; // Z extents

    float get_min_x(float char_x) const { return char_x + offset_x - half_width; }
    float get_max_x(float char_x) const { return char_x + offset_x + half_width; }
    float get_top_y(float char_y) const { return char_y + offset_y - half_height; }
    float get_bottom_y(float char_y) const { return char_y + offset_y + half_height; }
    float get_min_z(float char_z) const { return char_z + offset_z - half_depth; }
    float get_max_z(float char_z) const { return char_z + offset_z + half_depth; }

    AABB get_aabb(float char_x, float char_y, float char_z) const {
        return {
            get_min_x(char_x), get_max_x(char_x),
            get_top_y(char_y), get_bottom_y(char_y),
            get_min_z(char_z), get_max_z(char_z)
        };
    }
};

// =========================================================================
// BASE CHARACTER CLASS (Supports arbitrary X limbs, textures, hitbox & physics)
// =========================================================================
class character {
public:
    enum class State {
        IDLE,
        WALKING,
        JUMPING
    };

    float m_x_offset = 0.0f;
    float m_y_offset = 0.0f;
    float m_z_offset = 0.0f;

    float m_spawn_x = 0.0f;
    float m_spawn_y = 0.0f;
    float m_spawn_z = 0.0f;

    float m_angle = 0.0f;
    float m_target_angle = 0.0f;
    float m_move_speed = 0.15f;

    // Movement Intent for physical resolution
    float m_intended_dx = 0.0f;
    float m_intended_dz = 0.0f;

    // Physics & Jump variables (Note: in screen canvas, -Y is Up, +Y is Down)
    float m_vy = 0.0f;
    float m_gravity = 0.018f;
    float m_jump_force = -0.36f;
    bool m_is_grounded = false;

    // Collision & Simulation Flags
    bool m_is_solid = false;    // True if other actors collide with this (floors, walls, props)
    bool m_is_dynamic = false;  // True if this actor is updated by physics & gravity (player, enemies, dynamic items)
    bool m_has_gravity = true;  // True if affected by downward gravity

    State m_state = State::IDLE;
    float m_anim_time = 0.0f;

    // Hitbox for 3D AABB collision detection and floor check
    Hitbox m_hitbox;

    // Dynamic Limbs: Supports any number (X) of limbs
    std::vector<item_loader::mesh_actor> m_limbs;

    // Character Texture Decal (attached to a designated parent limb)
    bool m_has_texture = false;
    sf::Texture m_texture;
    std::string m_texture_path;
    int m_texture_parent_limb_idx = 0;
    float m_tex_rel_x = 0.0f;
    float m_tex_rel_y = 0.0f;
    float m_tex_rel_z = 1.02f; // Placed on front (+Z) of body sphere where feet point
    float m_tex_width = 1.2f;
    float m_tex_height = 1.2f;
    bool m_billboard = false;

    using GridPair = std::pair<std::vector<std::vector<float>>, std::vector<std::vector<int>>>;

    character(float x_offset = 0.0f, float y_offset = 0.0f, float z_offset = 0.0f)
        : m_x_offset(x_offset), m_y_offset(y_offset), m_z_offset(z_offset),
          m_spawn_x(x_offset), m_spawn_y(y_offset), m_spawn_z(z_offset) {}

    virtual ~character() = default;

    AABB get_aabb() const {
        return m_hitbox.get_aabb(m_x_offset, m_y_offset, m_z_offset);
    }

    void set_spawn_point(float x, float y, float z) {
        m_spawn_x = x;
        m_spawn_y = y;
        m_spawn_z = z;
    }

    virtual void respawn() {
        m_x_offset = m_spawn_x;
        m_y_offset = m_spawn_y;
        m_z_offset = m_spawn_z;
        m_vy = 0.0f;
        m_intended_dx = 0.0f;
        m_intended_dz = 0.0f;
        m_is_grounded = false;
        m_state = State::IDLE;
        sync_limbs();
    }

    void sync_limbs() {
        for (auto& limb : m_limbs) {
            limb.set_position(m_x_offset, m_y_offset, m_z_offset);
        }
    }

    // Adds a new limb to the character
    size_t add_limb(const std::string& obj_path, float rel_x, float rel_y, float rel_z, sf::Color color = sf::Color::White) {
        item_loader::mesh_actor limb(m_x_offset, m_y_offset, m_z_offset, color);
        limb.load_obj_from_disk(obj_path);
        limb.set_relative_position_to_parent(rel_x, rel_y, rel_z);
        m_limbs.push_back(std::move(limb));
        return m_limbs.size() - 1;
    }

    // Loads texture for the character
    bool load_texture(const std::string& file_path, int parent_limb = 0, float rel_x = 0.0f, float rel_y = 0.0f, float rel_z = 1.02f, float width = 1.2f, float height = 1.2f) {
        m_texture_path = file_path;
        m_texture_parent_limb_idx = parent_limb;
        m_tex_rel_x = rel_x;
        m_tex_rel_y = rel_y;
        m_tex_rel_z = rel_z;
        m_tex_width = width;
        m_tex_height = height;

        if (m_texture.loadFromFile(file_path)) {
            m_texture.setSmooth(true);
            m_has_texture = true;
            return true;
        }
        return false;
    }

    virtual void set_position(float x, float y, float z) {
        m_x_offset = x;
        m_y_offset = y;
        m_z_offset = z;
        sync_limbs();
    }

    virtual void set_rotation(float angle) {
        m_angle = angle;
        m_target_angle = angle;
        for (auto& limb : m_limbs) {
            limb.set_angle(angle);
        }
    }

    // Trigger jump
    virtual void jump() {
        if (m_is_grounded) {
            m_vy = m_jump_force;
            m_is_grounded = false;
            m_state = State::JUMPING;
        }
    }

    // Move character intent in 3D world space based on input directions
    virtual void move(float dx, float dz) {
        m_intended_dx += dx;
        m_intended_dz += dz;

        if (std::abs(dx) > 0.0001f || std::abs(dz) > 0.0001f) {
            if (m_is_grounded) {
                m_state = State::WALKING;
            }
            m_target_angle = std::atan2(dx, dz);

            float diff = m_target_angle - m_angle;
            while (diff > 3.14159265f) diff -= 6.2831853f;
            while (diff < -3.14159265f) diff += 6.2831853f;
            m_angle += diff * 0.25f;
        }
    }

    // High-level 3D collision resolution and physics simulation against solid colliders
    virtual void resolve_physics_and_collision(const std::vector<character*>& solid_colliders) {
        // Idle check if not moving and grounded
        if (std::abs(m_intended_dx) < 0.00001f && std::abs(m_intended_dz) < 0.00001f) {
            if (m_is_grounded && m_state != State::JUMPING) {
                m_state = State::IDLE;
            }
        }

        // 1. Resolve Horizontal Movement in X (Wall collision)
        if (std::abs(m_intended_dx) > 0.00001f) {
            float new_x = m_x_offset + m_intended_dx;
            AABB box_x = {
                m_hitbox.get_min_x(new_x), m_hitbox.get_max_x(new_x),
                m_hitbox.get_top_y(m_y_offset), m_hitbox.get_bottom_y(m_y_offset),
                m_hitbox.get_min_z(m_z_offset), m_hitbox.get_max_z(m_z_offset)
            };

            for (const auto* other : solid_colliders) {
                if (other == this) continue;
                AABB other_box = other->get_aabb();
                if (box_x.overlaps(other_box)) {
                    if (m_intended_dx > 0.0f) {
                        new_x = other_box.min_x - (m_hitbox.offset_x + m_hitbox.half_width);
                    } else {
                        new_x = other_box.max_x - (m_hitbox.offset_x - m_hitbox.half_width);
                    }
                    break;
                }
            }
            m_x_offset = new_x;
            m_intended_dx = 0.0f;
        }

        // 2. Resolve Horizontal Movement in Z (Wall collision)
        if (std::abs(m_intended_dz) > 0.00001f) {
            float new_z = m_z_offset + m_intended_dz;
            AABB box_z = {
                m_hitbox.get_min_x(m_x_offset), m_hitbox.get_max_x(m_x_offset),
                m_hitbox.get_top_y(m_y_offset), m_hitbox.get_bottom_y(m_y_offset),
                m_hitbox.get_min_z(new_z), m_hitbox.get_max_z(new_z)
            };

            for (const auto* other : solid_colliders) {
                if (other == this) continue;
                AABB other_box = other->get_aabb();
                if (box_z.overlaps(other_box)) {
                    if (m_intended_dz > 0.0f) {
                        new_z = other_box.min_z - (m_hitbox.offset_z + m_hitbox.half_depth);
                    } else {
                        new_z = other_box.max_z - (m_hitbox.offset_z - m_hitbox.half_depth);
                    }
                    break;
                }
            }
            m_z_offset = new_z;
            m_intended_dz = 0.0f;
        }

        // 3. Resolve Vertical Gravity & Y Movement (Floor landing & Ceiling collision)
        if (m_has_gravity) {
            if (!m_is_grounded) {
                m_vy += m_gravity;
                if (m_vy > 0.8f) m_vy = 0.8f; // Terminal velocity
            }
        }

        float prev_y = m_y_offset;
        float new_y = m_y_offset + m_vy;
        AABB box_y = {
            m_hitbox.get_min_x(m_x_offset), m_hitbox.get_max_x(m_x_offset),
            m_hitbox.get_top_y(new_y), m_hitbox.get_bottom_y(new_y),
            m_hitbox.get_min_z(m_z_offset), m_hitbox.get_max_z(m_z_offset)
        };

        bool landed = false;
        for (const auto* other : solid_colliders) {
            if (other == this) continue;
            AABB other_box = other->get_aabb();
            if (box_y.overlaps(other_box)) {
                if (m_vy >= 0.0f) {
                    // Falling down (+Y): Landing on top surface of other
                    float prev_bottom = m_hitbox.get_bottom_y(prev_y);
                    if (prev_bottom <= other_box.min_y + 0.6f) {
                        new_y = other_box.min_y - (m_hitbox.offset_y + m_hitbox.half_height);
                        m_vy = 0.0f;
                        m_is_grounded = true;
                        if (m_state == State::JUMPING) {
                            m_state = State::IDLE;
                        }
                        landed = true;
                        break;
                    }
                } else if (m_vy < 0.0f) {
                    // Jumping up (-Y): Bumping head against bottom surface
                    float prev_top = m_hitbox.get_top_y(prev_y);
                    if (prev_top >= other_box.max_y - 0.6f) {
                        new_y = other_box.max_y - (m_hitbox.offset_y - m_hitbox.half_height);
                        m_vy = 0.0f;
                        break;
                    }
                }
            }
        }

        m_y_offset = new_y;

        // 4. Ground Probe: If previously grounded, check if solid floor is still beneath feet
        if (m_is_grounded && !landed) {
            float probe_y = m_y_offset + 0.08f;
            AABB probe_box = {
                m_hitbox.get_min_x(m_x_offset) + 0.05f, m_hitbox.get_max_x(m_x_offset) - 0.05f,
                m_hitbox.get_top_y(probe_y), m_hitbox.get_bottom_y(probe_y),
                m_hitbox.get_min_z(m_z_offset) + 0.05f, m_hitbox.get_max_z(m_z_offset) - 0.05f
            };
            bool has_ground = false;
            for (const auto* other : solid_colliders) {
                if (other == this) continue;
                if (probe_box.overlaps(other->get_aabb())) {
                    has_ground = true;
                    break;
                }
            }
            if (!has_ground) {
                m_is_grounded = false;
                if (m_state != State::JUMPING) {
                    m_state = State::JUMPING;
                }
            }
        }

        // 5. Void Fall Detection & Respawn
        if (m_y_offset > 35.0f) {
            respawn();
        }

        // 6. Sync limb transforms
        sync_limbs();
    }

    // Draws all X limbs and bundles their colors into RenderPayloads
    virtual std::vector<renderer::RenderPayload> draw_self() {
        std::vector<renderer::RenderPayload> payloads;
        payloads.reserve(m_limbs.size());

        for (auto& limb : m_limbs) {
            auto mesh_data = limb.draw_self();
            renderer::RenderPayload p;
            p.vertices = std::move(mesh_data.first);
            p.triangles = std::move(mesh_data.second);
            p.base_color = limb.m_color;
            payloads.push_back(std::move(p));
        }
        return payloads;
    }

    // Computes texture render payloads attached to its designated limb
    virtual std::vector<renderer::TextureRenderPayload> draw_textures() const {
        std::vector<renderer::TextureRenderPayload> payloads;
        if (!m_has_texture || m_texture.getSize().x == 0 || m_texture.getSize().y == 0) {
            return payloads;
        }

        float limb_angle = m_angle;
        float local_x = m_tex_rel_x;
        float local_y = m_tex_rel_y;
        float local_z = m_tex_rel_z;

        if (m_texture_parent_limb_idx >= 0 && m_texture_parent_limb_idx < (int)m_limbs.size()) {
            const auto& parent_limb = m_limbs[m_texture_parent_limb_idx];
            limb_angle = parent_limb.m_angle;
            local_x = parent_limb.x_offset_relative_to_parent + m_tex_rel_x;
            local_y = parent_limb.y_offset_relative_to_parent + m_tex_rel_y;
            local_z = parent_limb.z_offset_relative_to_parent + m_tex_rel_z;
        }

        float cos_a = std::cos(limb_angle);
        float sin_a = std::sin(limb_angle);

        float rot_x = local_x * cos_a + local_z * sin_a;
        float rot_y = local_y;
        float rot_z = -local_x * sin_a + local_z * cos_a;

        renderer::TextureRenderPayload payload;
        payload.texture = &m_texture;
        payload.world_x = rot_x + m_x_offset;
        payload.world_y = rot_y + m_y_offset;
        payload.world_z = rot_z + m_z_offset;
        payload.width = m_tex_width;
        payload.height = m_tex_height;
        payload.angle = limb_angle;
        payload.billboard = m_billboard;
        payload.cull_backface = !m_billboard;

        payloads.push_back(payload);
        return payloads;
    }

    virtual void tick() {
        for (auto& limb : m_limbs) {
            limb.tick();
        }
    }
};

// =========================================================================
// KIRBY IMPLEMENTATION (Idle, Walk, and Jump Animations)
// =========================================================================
class kirby_character : public character {
public:
    kirby_character(float x_offset = 0.0f, float y_offset = 0.0f, float z_offset = 0.0f,
                    const std::string& texture_path = "assets/textures/kirby_face.png",
                    sf::Color body_color = sf::Color(255, 182, 193),
                    sf::Color foot_color = sf::Color(220, 20, 60))
        : character(x_offset, y_offset, z_offset)
    {
        m_spawn_x = x_offset;
        m_spawn_y = y_offset;
        m_spawn_z = z_offset;

        m_is_dynamic = true;
        m_is_solid = false;
        m_has_gravity = true;

        // Hitbox: height 1.0f (feet at Y=+1.0f, head at Y=-1.0f), width 0.85f
        m_hitbox.offset_x = 0.0f;
        m_hitbox.offset_y = 0.0f;
        m_hitbox.offset_z = 0.0f;
        m_hitbox.half_width = 0.85f;
        m_hitbox.half_height = 1.0f;
        m_hitbox.half_depth = 0.85f;

        // Limb 0: Torso Body (Ball)
        add_limb("assets/kirby_parts/ball.obj", 0.0f, 0.0f, 0.0f, body_color);
        // Limb 1: Left Foot
        add_limb("assets/kirby_parts/kirby_foot.obj", -1.0f, 1.0f, 0.0f, foot_color);
        // Limb 2: Right Foot
        add_limb("assets/kirby_parts/kirby_foot.obj", 1.0f, 1.0f, 0.0f, foot_color);

        // Load face texture attached to front of torso (Limb 0)
        if (!load_texture(texture_path, 0, 0.0f, 0.0f, 1.02f, 1.2f, 1.2f)) {
            load_texture("assets/textures/face.png", 0, 0.0f, 0.0f, 1.02f, 1.2f, 1.2f);
        }
    }

    void tick() override {
        if (m_limbs.size() < 3) return;

        auto& torso = m_limbs[0];
        auto& left_foot = m_limbs[1];
        auto& right_foot = m_limbs[2];

        if (m_state == State::JUMPING) {
            // =================================================================
            // JUMP ANIMATION: Float, puff up, limbs spread joyfully in air
            // =================================================================
            m_anim_time += 0.15f;

            // In mid-air, Kirby stretches and puffs up
            float air_bob = std::sin(m_anim_time) * 0.05f;
            torso.x_offset_relative_to_parent = 0.0f;
            torso.y_offset_relative_to_parent = -0.1f + air_bob;
            torso.z_offset_relative_to_parent = 0.0f;
            torso.m_angle = m_angle;

            // Feet kick upwards / spread wide while flying/jumping
            left_foot.x_offset_relative_to_parent = -1.15f;
            left_foot.y_offset_relative_to_parent = 0.6f + std::sin(m_anim_time * 2.0f) * 0.08f;
            left_foot.z_offset_relative_to_parent = 0.15f;
            left_foot.m_angle = m_angle - 0.2f;

            right_foot.x_offset_relative_to_parent = 1.15f;
            right_foot.y_offset_relative_to_parent = 0.6f - std::sin(m_anim_time * 2.0f) * 0.08f;
            right_foot.z_offset_relative_to_parent = 0.15f;
            right_foot.m_angle = m_angle + 0.2f;

        } else if (m_state == State::WALKING) {
            // =================================================================
            // WALK ANIMATION: Energetic waddle, stride, and step bounce
            // =================================================================
            m_anim_time += 0.22f;

            float step_bounce = std::abs(std::sin(m_anim_time)) * -0.15f;
            float waddle_roll = std::sin(m_anim_time * 0.5f) * 0.12f;
            float waddle_x    = std::sin(m_anim_time * 0.5f) * 0.06f;

            torso.x_offset_relative_to_parent = waddle_x;
            torso.y_offset_relative_to_parent = step_bounce;
            torso.z_offset_relative_to_parent = 0.0f;
            torso.m_angle = m_angle + waddle_roll;

            float stride_l = std::sin(m_anim_time) * 0.5f;
            float stride_r = -std::sin(m_anim_time) * 0.5f;
            float lift_l   = std::max(0.0f, std::sin(m_anim_time)) * -0.25f;
            float lift_r   = std::max(0.0f, -std::sin(m_anim_time)) * -0.25f;

            left_foot.x_offset_relative_to_parent = -1.0f;
            left_foot.y_offset_relative_to_parent = 1.0f + lift_l;
            left_foot.z_offset_relative_to_parent = stride_l;
            left_foot.m_angle = m_angle;

            right_foot.x_offset_relative_to_parent = 1.0f;
            right_foot.y_offset_relative_to_parent = 1.0f + lift_r;
            right_foot.z_offset_relative_to_parent = stride_r;
            right_foot.m_angle = m_angle;

        } else {
            // =================================================================
            // IDLE ANIMATION: Gentle breathing bob and weight shift
            // =================================================================
            m_anim_time += 0.05f;

            float body_bounce = std::sin(m_anim_time * 2.5f) * 0.08f;
            float body_sway_x = std::sin(m_anim_time * 1.25f) * 0.03f;
            float body_rot    = m_angle + std::sin(m_anim_time * 1.25f) * 0.12f;

            torso.x_offset_relative_to_parent = body_sway_x;
            torso.y_offset_relative_to_parent = body_bounce;
            torso.z_offset_relative_to_parent = 0.0f;
            torso.m_angle = body_rot;

            left_foot.x_offset_relative_to_parent = -1.0f - (body_bounce * 0.25f);
            left_foot.y_offset_relative_to_parent = 1.0f + std::sin(m_anim_time * 2.5f + 0.5f) * 0.03f;
            left_foot.z_offset_relative_to_parent = std::cos(m_anim_time * 1.25f) * 0.04f;
            left_foot.m_angle = m_angle + std::sin(m_anim_time * 1.25f + 0.3f) * 0.08f;

            right_foot.x_offset_relative_to_parent = 1.0f + (body_bounce * 0.25f);
            right_foot.y_offset_relative_to_parent = 1.0f + std::cos(m_anim_time * 2.5f + 0.5f) * 0.03f;
            right_foot.z_offset_relative_to_parent = -std::cos(m_anim_time * 1.25f) * 0.04f;
            right_foot.m_angle = m_angle + std::sin(m_anim_time * 1.25f - 0.3f) * 0.08f;
        }

        torso.tick();
        left_foot.tick();
        right_foot.tick();
    }
};

// =========================================================================
// GROUND PLATFORM ACTOR (Flat Grass Ground Platform with Grass Texture)
// =========================================================================
class ground_actor : public character {
public:
    float m_surface_y = 1.0f; // Top surface level of the platform
    float m_platform_half_size = 30.0f;
    sf::Texture m_grass_texture;
    bool m_has_grass_texture = false;

    ground_actor(float x_offset = 0.0f, float y_offset = 0.0f, float z_offset = 0.0f,
                 sf::Color grass_color = sf::Color(65, 175, 75))
        : character(x_offset, y_offset, z_offset)
    {
        m_is_solid = true;
        m_is_dynamic = false;
        m_has_gravity = false;

        // Hitbox representing the 3D flat platform stage (Mesh extends from Y=1 to Y=4)
        m_hitbox.offset_x = 0.0f;
        m_hitbox.offset_y = 2.5f; // Center of Y=[1.0, 4.0]
        m_hitbox.offset_z = 0.0f;
        m_hitbox.half_width = m_platform_half_size;
        m_hitbox.half_height = 1.5f; // Top is 2.5 - 1.5 = 1.0f, Bottom is 2.5 + 1.5 = 4.0f
        m_hitbox.half_depth = m_platform_half_size;

        // Add 3D flat platform mesh
        add_limb("assets/ground_platform.obj", 0.0f, 0.0f, 0.0f, grass_color);

        // Load Grass texture
        if (m_grass_texture.loadFromFile("assets/textures/grass.png")) {
            m_grass_texture.setSmooth(true);
            m_grass_texture.setRepeated(true);
            m_has_grass_texture = true;
        }
    }

    // Overlay grass texture patches across the flat platform stage
    std::vector<renderer::TextureRenderPayload> draw_textures() const override {
        std::vector<renderer::TextureRenderPayload> payloads;
        if (!m_has_grass_texture) return payloads;

        const int grid_x = 5;
        const int grid_z = 5;
        const float spacing = 11.0f;

        for (int gz = 0; gz < grid_z; ++gz) {
            float pz = m_z_offset - 22.0f + gz * spacing;
            for (int gx = 0; gx < grid_x; ++gx) {
                float px = m_x_offset - 22.0f + gx * spacing;

                renderer::TextureRenderPayload payload;
                payload.texture = &m_grass_texture;
                payload.world_x = px;
                payload.world_y = m_y_offset + m_surface_y - 0.02f;
                payload.world_z = pz;
                payload.width = 6.5f;
                payload.height = 6.5f;
                payload.angle = 0.0f;
                payload.tint = sf::Color(255, 255, 255, 220);
                payload.is_horizontal_plane = true; // Lay flat horizontally on top of the ground platform
                payload.billboard = false;
                payload.cull_backface = false;

                payloads.push_back(payload);
            }
        }

        return payloads;
    }

    void tick() override {
        if (!m_limbs.empty()) {
            m_limbs[0].tick();
        }
    }
};

using world_cylinder_actor = ground_actor;

// =========================================================================
// CLOUD ACTOR (Drifting Fluffy Sky Clouds in the Bright Blue Sky)
// =========================================================================
class cloud_actor : public character {
public:
    float m_drift_speed = 0.02f;
    float m_min_x = -40.0f;
    float m_max_x =  40.0f;

    cloud_actor(float x, float y, float z, float width = 7.0f, float height = 3.5f, float speed = 0.02f)
        : character(x, y, z), m_drift_speed(speed)
    {
        m_is_solid = false;
        m_is_dynamic = false;
        m_has_gravity = false;

        load_texture("assets/textures/cloud.png", -1, 0.0f, 0.0f, 0.0f, width, height);
        m_billboard = true;
    }

    void tick() override {
        m_x_offset += m_drift_speed;
        if (m_x_offset > m_max_x) {
            m_x_offset = m_min_x;
        }
    }

    std::vector<renderer::RenderPayload> draw_self() override {
        // Clouds are purely textured billboards, no solid mesh
        return {};
    }
};

// =========================================================================
// RETRO COMPUTER TERMINAL ACTOR (Interactive 3D Workstation in the World)
// =========================================================================
class terminal_actor : public character {
public:
    terminal_actor(float x_offset = 4.0f, float y_offset = 0.0f, float z_offset = 4.0f)
        : character(x_offset, y_offset, z_offset)
    {
        m_is_solid = true;
        m_is_dynamic = false;
        m_has_gravity = false;

        // Hitbox for terminal computer desk and monitor (from Y=-1.6 to Y=1.0)
        m_hitbox.offset_x = 0.0f;
        m_hitbox.offset_y = -0.3f;
        m_hitbox.offset_z = 0.0f;
        m_hitbox.half_width = 1.1f;
        m_hitbox.half_height = 1.3f; // Top is -0.3 - 1.3 = -1.6f, Bottom is -0.3 + 1.3 = +1.0f
        m_hitbox.half_depth = 1.1f;

        // Limb 0: Computer table, case, monitor, keyboard
        add_limb("assets/terminal_computer.obj", 0.0f, 0.0f, 0.0f, sf::Color(215, 210, 195));

        // CRT Screen decal placed on the front face of the monitor
        load_texture("assets/textures/terminal_screen.png", 0, 0.0f, -0.8f, 0.42f, 1.0f, 0.8f);
    }
};

using kirby = kirby_character;

// =========================================================================
// HIGH LEVEL PHYSICS PIPELINE
// =========================================================================
namespace physics {
    inline void update_world(std::vector<std::unique_ptr<character>>& actor_vec) {
        // 1. Gather all solid colliders
        std::vector<character*> solid_colliders;
        solid_colliders.reserve(actor_vec.size());
        for (auto& act : actor_vec) {
            if (act && act->m_is_solid) {
                solid_colliders.push_back(act.get());
            }
        }

        // 2. Tick and update physics for all actors
        for (auto& act : actor_vec) {
            if (!act) continue;

            // Internal animation and movement tick
            act->tick();

            // Dynamic physics simulation and 3D collision resolution
            if (act->m_is_dynamic) {
                act->resolve_physics_and_collision(solid_colliders);
            }
        }
    }
}
