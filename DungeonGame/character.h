#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include <vector>

#include "dims.h"


class dungeon_t;
class npc_t;
class pc_t;

typedef struct dice_t dice_t;

typedef enum kill_type {
    kill_direct,
    kill_avenged,
    num_kill_types
  } kill_type_t;
  

class character_t{
public:
    char symbol;
    pair_t position;
    int32_t speed;
    uint32_t alive;
    uint32_t sequence_number;
    uint32_t quit;
    npc_t *npc;
    pc_t *pc;
    uint32_t kills[num_kill_types];
    std::vector<char> DAM;
    int HP;

    //Constructor
    character_t() : symbol('\0'), speed(0), alive(true), sequence_number(0), quit(false), npc(nullptr), pc(nullptr) // Declare parameters here
        //: symbol(sym), speed(spd), alive(true), sequence_number(seq_num), quit(false), npc(nullptr), pc(nullptr) 
        {
        // Constructor body (if needed)s
    }
    
    //Virtual Destructor
    ~character_t() {}


    //TODO: Do I need this?
    //int32_t compare_characters_by_next_turn(const character_t *other);

    uint32_t can_see(dungeon_t *d, character_t *other);

    void character_delete();


    uint32_t dungeon_has_npcs(dungeon_t *d);


    void npc_next_pos(dungeon_t *d, int8_t *next);

    int validCharacter(){
        if(speed > 0){
            return 1;
        }else{
            return 0;
        }
    }


    void increaseSpeed(int spd){
        speed += spd;
    }
    
    void descreaseSpeed(int spd){
        speed -= spd;
    }

    void increaseHP(int h){
        HP += h;
    }

    void decreaseHP(int h){
        HP -= h;
    }


};



#endif

