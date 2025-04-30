#ifndef PC_H
#define PC_H

#include <stdint.h>
#include <array>

#include "character.h"
#include "dims.h"
#include "objects.h"

class dungeon_t;

enum class EquipmentSlot {
    WEAPON,
    OFFHAND,
    RANGED,
    ARMOR,
    HELMET,
    CLOAK,
    GLOVES,
    BOOTS,
    AMULET,
    LIGHT,
    RING1,
    RING2,
    COUNT
};

class pc_t : public character_t {
private:
    static const int CARRY_CAPACITY = 10;
    std::array<Objects*, static_cast<size_t>(EquipmentSlot::COUNT)> equipment{nullptr};


public:

    std::array<Objects*, CARRY_CAPACITY> carry{nullptr};
    int WIN;


    //Constructor
    pc_t() : character_t(){//: character_t('@', 10, 1) {  // Call the character_t constructor with parameters
        // Additional initialization for pc_t if needed
        this->HP = 500;
        this->DAM = {'0', '+', '1', 'd', '4'};
        this->pc = this;
        this->npc = nullptr;
        WIN = 0;
    }

    //Virtual Destructor
    ~pc_t() {}

    const std::array<Objects*, static_cast<size_t>(EquipmentSlot::COUNT)>& get_equipment() const {
        return equipment;
    }
   
    
    Objects* get_equipped_item(EquipmentSlot slot) const {
        return equipment[static_cast<size_t>(slot)];
    }

    void remove_equipped_item(EquipmentSlot slot) {
        equipment[static_cast<size_t>(slot)] = nullptr;
    }

    void pc_delete();
    uint32_t pc_is_alive(dungeon_t *d);
    void config_pc(dungeon_t *d);
    uint32_t teleporting_mode(dungeon_t *d, pair_t dir, char **error_message);
    uint32_t pc_next_pos(dungeon_t *d, pair_t dir, int keyPressed, char ** error_message, int *teleporting, char **notifications);
    void place_pc(dungeon_t *d);
    uint32_t pc_in_room(dungeon_t *d, uint32_t room);
    uint32_t pc_quit(dungeon_t *d);
    uint32_t inspect_mode(dungeon_t *d, pair_t dir, char **error_message, char **notifications);

    //TODO: Need to handle check and copying equipment to open carry outside thie call
    void set_equipment(EquipmentSlot slot, Objects* obj){

        //Deletes equipment currently in spot then places new equipment
        if (equipment[static_cast<size_t>(slot)]) {
            delete equipment[static_cast<size_t>(slot)];
        }
        equipment[static_cast<size_t>(slot)] = obj;
    }



    //Add object to carry inventory
    bool add_to_carry(Objects* obj) {
        for (int i = 0; i < CARRY_CAPACITY; ++i) {
            if (carry[i] == nullptr) {
                carry[i] = obj;
                return true; // Successfully added
            }
        }
        return false; // Carry array is full
    }


    int win(){
        return WIN;
    }
    

};

#endif


// #ifndef PC_H
// # define PC_H

// # include <stdint.h>

// # include "dims.h"

// typedef struct dungeon dungeon_t;

// typedef struct pc {
// } pc_t;

// void pc_delete(pc_t *pc);
// uint32_t pc_is_alive(dungeon_t *d);
// void config_pc(dungeon_t *d);
// uint32_t pc_next_pos(dungeon_t *d, pair_t dir, int keyPressed, char **error_message);
// void place_pc(dungeon_t *d);
// uint32_t pc_in_room(dungeon_t *d, uint32_t room);
// uint32_t pc_quit(dungeon_t *d);

// #endif
