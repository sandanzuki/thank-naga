#ifndef UNIT_H
#define UNIT_H

#include <cstdlib>
#include <vector>
#include <string>
#include <utility>
#include "Effect.hpp"

using namespace std;

enum UnitType
{
    FIGHTER,
    ARCHER,
    MAGE,
    HEALER,
    INVALID
};

UnitType string_to_unit_type(string st);
string unit_type_to_string(UnitType type);

class Unit
{
    public:
        // Constructor
        Unit(int _unit_id, UnitType _type, int _player_id, int x, int y);

        // Getters
        int get_unit_id() const { return unit_id; }
        UnitType get_type() const { return type; }
        string get_type_string() const { return unit_type_to_string(type); }
        int get_player_id() const { return player_id; }
        int get_move_distance() const { return move_distance; }
        int get_remaining_health() const { return remaining_health; }
        int get_max_health() { return max_health; }
        int get_x() { return x; }
        int get_y() { return y; }
        bool has_interacted() { return interacted; }
        bool has_moved() { return moved || interacted; }

        // Setters
        void new_turn() { interacted = false; moved = false; }
        void set_position(int _x, int _y) { x = _x; y = _y; moved = true; }
        void set_interacted() { interacted = true; }

        // Hit/Damage/Heal Calculations
        bool interact(Unit *target);    // TRUE if successful, FALSE if illegal
        void apply_damage(Unit *attacker, bool counter);
        void apply_heal();

        // Status
        bool is_alive() { return remaining_health > 0; }
        bool is_within_range(Unit *target);
        
    private:
        // Unit Indentification Information
        int unit_id;
        UnitType type;
        int player_id;
        int move_distance;

        // Unit Gameplay Information
        int remaining_health;
        int max_health;
        int x;
        int y;
        bool interacted;
        bool moved;
};

#endif
