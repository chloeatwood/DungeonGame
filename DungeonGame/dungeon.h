#ifndef DUNGEON_H
# define DUNGEON_H

# include "heap.h"
# include "dims.h"
# include "character.h"
# include "pc.h"
#include "monsterDescription.h"
#include "itemDescription.h"
#include "objects.h"
#include "npc.h"

#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <cassert>

#define DUNGEON_X              80
#define DUNGEON_Y              21
#define MIN_ROOMS              6
#define MAX_ROOMS              10
#define ROOM_MIN_X             4
#define ROOM_MIN_Y             3
#define ROOM_MAX_X             20
#define ROOM_MAX_Y             15
#define VISUAL_RANGE           15
#define PC_SPEED               10
#define NPC_MIN_SPEED          5
#define NPC_MAX_SPEED          15
#define MAX_MONSTERS           15
#define SAVE_DIR               ".rlg327"
#define DUNGEON_SAVE_FILE      "dungeon"
#define DUNGEON_SAVE_SEMANTIC  "RLG327-" TERM
#define DUNGEON_SAVE_VERSION   0U
#define MAX_OBJECTS 25
#define MIN_OBJECTS 10

//#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
//#define mapxy(x, y) (d->map[y][x])
//#define hardnesspair(pair) (d->hardness[pair[dim_y]][pair[dim_x]])
//#define hardnessxy(x, y) (d->hardness[y][x])
//#define charpair(pair) (d->character[pair[dim_y]][pair[dim_x]])
//#define charxy(x, y) (d->character[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_debug,
  ter_wall,
  ter_wall_immutable,
  ter_floor,
  ter_floor_room,
  ter_floor_hall,
  ter_stairs,
  ter_stairs_up,
  ter_stairs_down,
  ter_not_seen_by_player
} terrain_type_t;


typedef struct room {
    pair_t position;
    pair_t size;
  } room_t;

class dungeon_t {
public:
    uint32_t num_rooms;
    room_t *rooms;
    terrain_type_t map[DUNGEON_Y][DUNGEON_X];
    terrain_type_t seenByPlayer[DUNGEON_Y][DUNGEON_X];
    /* Since hardness is usually not used, it would be expensive to pull it *
     * into cache every time we need a map cell, so we store it in a        *
     * parallel array, rather than using a structure to represent the       *
     * cells.  We may want a cell structure later, but from a performanace  *
     * perspective, it would be a bad idea to ever have the map be part of  *
     * that structure.  Pathfinding will require efficient use of the map,  *
     * and pulling in unnecessary data with each map cell would add a lot   *
     * of overhead to the memory system.                                    */
    uint8_t hardness[DUNGEON_Y][DUNGEON_X];
    uint8_t pc_distance[DUNGEON_Y][DUNGEON_X];
    uint8_t pc_tunnel[DUNGEON_Y][DUNGEON_X];
    character_t *character[DUNGEON_Y][DUNGEON_X];
    npc_t *npc;
    pc_t* pc;
    heap_t events;
    uint16_t num_monsters;
    uint16_t max_monsters;
    uint32_t character_sequence_number;
    uint32_t quit;
    std::vector<monsterDescription_t> *possible_monsters;
    std::vector<itemDescription_t> *possible_items;
    Objects *game_objects[DUNGEON_Y][DUNGEON_X];
    //Object will be added to this list if it has been created and another instance can never exist again
    std::vector<Objects> *unique_objects;

    //Monsters will be added to this list if it has been created and another instance can never exist again
    std::vector<npc_t*> *extinct_monsters;

    /* Game time isn't strictly necessary.  It's implicit in the turn number *
     * of the most recent thing removed from the event queue; however,       *
     * including it here--and keeping it up to date--provides a measure of   *
     * convenience, e.g., the ability to create a new event without explicit *
     * information from the current event.                                   */
    uint32_t time;
    uint32_t is_new;  

    dungeon_t() {
      possible_monsters = new std::vector<monsterDescription_t>();
      possible_items = new std::vector<itemDescription_t>();
      unique_objects = new std::vector<Objects>();
      extinct_monsters = new std::vector<npc_t*>();
    
      for (int y = 0; y < DUNGEON_Y; y++) {
        for (int x = 0; x < DUNGEON_X; x++) {
          character[y][x] = nullptr;
          hardness[y][x] = 0;
          pc_distance[y][x] = 0;
          pc_tunnel[y][x] = 0;
          map[y][x] = terrain_type_t();
          seenByPlayer[y][x] = ter_not_seen_by_player;
          game_objects[y][x] = nullptr;
        }
      }
    }
    

    ~dungeon_t() {
      // Free dynamically allocated memory for the vectors
      if(possible_monsters != nullptr){
        delete possible_monsters;
      }
      if(possible_items != nullptr){
        delete possible_items;
      }
      if(unique_objects != nullptr){
        delete unique_objects;
      }
      if(extinct_monsters != nullptr){
        delete extinct_monsters;
      }


      // Delete all characters and game objects in the dungeon
      for (int y = 0; y < DUNGEON_Y; y++) {
        for (int x = 0; x < DUNGEON_X; x++) {
            // character_t* m = character[y][x];
            // if (m != nullptr) {
            //     if (m->npc != nullptr) {
            //         delete m->npc;
            //         m->npc = nullptr;
            //     }
            //     delete m;
            //     character[y][x] = nullptr;
            // }
    
            if (game_objects[y][x] != nullptr) {
                delete game_objects[y][x];
                game_objects[y][x] = nullptr;
            }
        }
      }
      
    }
    

    void init_dungeon();
    void delete_dungeon();
    int gen_dungeon();
    void render_dungeon(char **error_message);


    
    void render_player_version_dungeon(char **error_message, char **notifications);
    void update_player_view_dungeon();

    int write_dungeon(char *file);
    int read_dungeon(char *file);
    int read_pgm(char *pgm);

    void render_distance_map();
    void render_tunnel_distance_map();
    void render_hardness_map();
    void render_movement_cost_map();
    

    int display_selection_prompt(const std::vector<std::string>& options, const std::string& title);
    void prompt_wear_item(char **notifications);
    void prompt_take_off_item();
    void prompt_drop_item();
    void display_monster_list();


    void display_pc_carry();
    void display_pc_equipment();
    void prompt_inspect_item();
    void prompt_view_item();

    int replace_dungeon();
    void clear_dungeon_for_reset();

    void gen_monsters();
    void gen_objects();

    void display_pc_stats();
    void display_message_popup(const std::vector<std::string>& message, const std::string& title);

    uint32_t max_monster_cells(){
      uint32_t i;
      uint32_t sum;
    
      for (i = sum = 0; i < this->num_rooms; i++) {
        if (!this->pc->pc_in_room(this, i)) {
          sum += this->rooms[i].size[dim_y] * this->rooms[i].size[dim_x];
        }
      }
    
      return sum;
  }

  void read_monster_descriptions(std::string fileName);

  void read_item_descriptions(std::string fileName);

  Objects* isObjecthere(int x, int y){
    return this->game_objects[y][x];
  }

  bool transfer_to_carry(Objects* obj, int carryCapacity) {
    // Try to add to carry
    for (int i = 0; i < carryCapacity; ++i) {
        if (this->pc->carry[i] == nullptr) {
            this->pc->carry[i] = obj;

            // Remove from game_objects if present
            for (int y = 0; y < DUNGEON_Y; ++y) {
              for (int x = 0; x < DUNGEON_X; ++x) {
                  if (game_objects[y][x] == obj) {
                      game_objects[y][x] = nullptr;
                      break;
                  }
              }
          }
          

            return true;
        }
    }
    return false; // Carry is full
  }

};

// Standalone functions for accessing dungeon data
static inline terrain_type_t mapxy(dungeon_t *d, int x, int y) {
  return d->map[y][x];
}

static inline uint8_t hardnessxy(dungeon_t *d, int x, int y) {
  return d->hardness[y][x];
}

static inline character_t* charxy(dungeon_t *d, int x, int y) {
  return d->character[y][x];
}

static inline character_t* playerMonsxy(dungeon_t *d, int x, int y){
  return d->character[y][x];
}

// Overloaded versions for using `pair_t`
static inline terrain_type_t& mappair(dungeon_t *d, const pair_t& p) {
  return d->map[p[dim_y]][p[dim_x]];
}

static inline uint8_t& hardnesspair(dungeon_t *d, const uint8_t p[2]) {
  return d->hardness[p[dim_y]][p[dim_x]];
}

static inline uint8_t& hardnesspairAnotherVersion(dungeon_t *d, const pair_t& p){
  return d->hardness[p[dim_y]][p[dim_x]];
}

static inline uint8_t hardnesspairOther(dungeon_t *d, const pair_t& p) {
  return d->hardness[p[dim_y]][p[dim_x]]; // Ensure this returns uint8_t
}

static inline character_t* charpairOther(dungeon_t *d, const uint8_t p[2]) {
  // Check if coordinates are within bounds
  if (p[dim_y] >= d->num_rooms || p[dim_x] >= d->num_rooms) {
      std::cerr << "Error: Coordinates out of bounds" << std::endl;
      return nullptr;
  }
  
  // Check if a character exists at the given position
  if (d->character[p[dim_y]][p[dim_x]] == nullptr) {
      std::cerr << "Error: No character at position (" << (int)p[dim_y] << ", " << (int)p[dim_x] << ")" << std::endl;
      return nullptr;
  }
  
  return d->character[p[dim_y]][p[dim_x]];
}


static inline character_t* charpair(dungeon_t *d, const pair_t& p) {
  return d->character[p[dim_y]][p[dim_x]];
}




static inline terrain_type_t& playerxy(dungeon_t *d, int x, int y){
  return d->seenByPlayer[y][x];
}

static inline terrain_type_t& playerPair(dungeon_t *d, const pair_t& p){
  return d->seenByPlayer[p[dim_y]][p[dim_x]];
}





#endif
