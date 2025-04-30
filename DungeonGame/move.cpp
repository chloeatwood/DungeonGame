#include "move.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ncurses.h>
#include <cstring>
#include <cstdlib>


#include "dungeon.h"
#include "heap.h"
#include "move.h"
#include "npc.h"
#include "pc.h"
#include "character.h"
#include "utils.h"
#include "path.h"
#include "event.h"

void do_combat(dungeon_t *d, character_t *atk, character_t *def, char **notifications) {

  // Suicide is bad. Preventing that
  if (def->npc && atk->npc) {
    // Monsters are friends. No attacking your friends
  } else {
          // If npc vs pc extra check
    if (def->pc || atk->pc) {
      // Base damage for the attacker: roll the dice for base damage string (atk->DAM)
      int total_damage = parseDice(atk->DAM).rollDaDice();  // Roll base damage first

      // If the attacker is the PC, roll damage for all equipped items
      if (atk->pc) {
        pc_t* pc = static_cast<pc_t*>(atk);

        // Roll for each equipped item
        for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::COUNT); i++) {
          Objects* equipped_item = pc->get_equipped_item(static_cast<EquipmentSlot>(i));
          if (equipped_item) {
            // Roll the damage dice for this equipped item and add it to the total damage
            total_damage += parseDice(equipped_item->DAM).rollDaDice();
          }
        }
        if (def->npc) {
          std::string msg = "The def " + std::string(def->npc->NAME) + " is being attacked. You are doing " + std::to_string(total_damage) + " damage";
          *notifications = strdup(msg.c_str());
        }
      }

      // Decrease defender's HP by total damage
      def->decreaseHP(total_damage);

      // If defender's HP falls below zero, they die
      if (def->HP <= 0) {
        def->alive = 0;
        // Handle NPC death
        if (def != d->pc) {
          d->num_monsters--;
          if (def->npc->isUnique() == 1) {
            d->extinct_monsters->push_back(def->npc);
          }
        }

        // Add to the attacker's kill count
        atk->kills[kill_direct]++;
        atk->kills[kill_avenged] += (def->kills[kill_direct] + def->kills[kill_avenged]);

        if (def->alive == 0 && def->npc != nullptr) {
          if(def->npc->isBoss()){
            if (atk == d->pc) {
              d->pc->WIN = 1;
              std::string msg = "Win value: " + std::to_string(d->pc->WIN);
              *notifications = strdup(msg.c_str());
            }
          }
          def->npc = nullptr;
        }
      }
    }

    }
}


void move_character(dungeon_t *d, character_t *c, pair_t next, char **notifications)
{
  // Check if next is within bounds
  if (next[dim_y] < 0 || next[dim_y] >= 21 || next[dim_x] < 0 || next[dim_x] >= 80) {
    // Handle out-of-bounds access (e.g., return or display error)
    return;
  }


  character_t *defender = d->character[next[dim_y]][next[dim_x]];

  if (defender) {  // There is a character in the target position
    do_combat(d, c, defender, notifications);  // Attacker = `c`, Defender = character in `next`

    if (!defender->alive) {  // If the defender died, remove it from the dungeon
      d->character[next[dim_y]][next[dim_x]] = NULL;
    } else {
      // If the defender survived (this shouldn't happen), do not move the attacker
      return;
    }
  }

      // Ensure d and c are not nullptr
      if (d == nullptr || c == nullptr) {
        fprintf(stderr, "Error: Invalid dungeon or character pointer in move_character\n");
        return;
    }

    // Check if c->position is valid
    if (c->position[dim_y] < 0 || c->position[dim_y] >= DUNGEON_Y || 
        c->position[dim_x] < 0 || c->position[dim_x] >= DUNGEON_X) {
        fprintf(stderr, "Error: Invalid position for character\n");
        return;
    }




  // Move the attacker into the target position
  d->character[c->position[dim_y]][c->position[dim_x]] = nullptr; // Remove from old spot
  c->position[dim_y] = next[dim_y];
  c->position[dim_x] = next[dim_x];
  d->character[next[dim_y]][next[dim_x]] = c; // Place in new spot
}





void do_moves(dungeon_t *d, int keyPressed, char **error_message, char **notifications)
{

  
  pair_t next;
  //pair_t *pc_next;
  character_t *c;
  event_t *e;
  int teleporting = 0;

  /* Remove the PC when it is PC turn.  Replace on next call.  This allows *
   * use to completely uninit the heap when generating a new level without *
   * worrying about deleting the PC.                                       */


  if (d->pc->pc_is_alive(d)) {
    /* The PC always goes first one a tie, so we don't use new_event().  *
     * We generate one manually so that we can set the PC sequence       *
     * number to zero.                                                   */
    e = (event_t*)malloc(sizeof (*e));
    e->type = event_character_turn;
    /* Hack: New dungeons are marked.  Unmark and ensure PC goes at d->time, *
     * otherwise, monsters get a turn before the PC.                         */
    if (d->is_new) {
      d->is_new = 0;
      e->time = d->time;
    } else {
      e->time = d->time + (1000 / d->pc->speed);
    }
    e->sequence = 0;
    e->c = d->pc;
    heap_insert(&d->events, e);
  }

  //While the pc is alive
  while (d->pc->pc_is_alive(d) &&
         (e = (event_t*)heap_remove_min(&d->events)) &&
         ((e->type != event_character_turn) || (e->c != d->pc))) {

          
    //If the current players type is a monsters/character
    d->time = e->time;

    if(e->type != event_character_turn){
      event_delete(e);
      continue;
    }
    c = e->c;
    // if (e->type == event_character_turn) {
    //   c = e->c;
    // }
    if (!c->alive && c->validCharacter()) {
      if (d->character[c->position[dim_y]][c->position[dim_x]] == c) {
        d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
      }
      if (c != d->pc) {
        delete c;
        event_delete(e);   // deletes e->c too
        e = NULL;
        continue;
      }
    }

    //Gets monsters next position
    c->npc_next_pos(d, next);

    //Moves the monster to the next position
    move_character(d, c, next, notifications);

    if (c->alive && c->speed > 0) {
      heap_insert(&d->events, update_event(d, e, 1000 / c->speed));
    } else {
      if (c != d->pc) {
        event_delete(e);
      }
    }
  }



  //If the player is alive
  //If the current player is a pc
  if (d->pc->pc_is_alive(d) && e->c == d->pc) {


    c = e->c;
    d->time = e->time;
    /* Kind of kludgey, but because the PC is never in the queue when   *
     * we are outside of this function, the PC event has to get deleted *
     * and recreated every time we leave and re-enter this function.    */
    e->c = NULL;
    event_delete(e);

    //get players next position
    d->pc->pc_next_pos(d, next, keyPressed, error_message, &teleporting, notifications);




      next[dim_x] += c->position[dim_x];
      next[dim_y] += c->position[dim_y];

      //Check if move is within bounds
      if(teleporting == 0){
        if(next[dim_y] < DUNGEON_Y && next[dim_x] < DUNGEON_X && next[dim_y] > 0 && next[dim_x] > 0){
          *error_message = NULL;
  
          if(hardnesspairAnotherVersion(d, next) == 1 || hardnesspairAnotherVersion(d, next) == 0){
            move_character(d, c, next, notifications);
          }else{
            next[dim_x] -= c->position[dim_x];
            next[dim_y] -= c->position[dim_y];
            *error_message = const_cast<char*>("Cannot move there. There is a wall in the way\n");
          }
  
  
        }else{
          next[dim_x] -= c->position[dim_x];
          next[dim_y] -= c->position[dim_y];
          *error_message = const_cast<char*>("Cannot move there. There is a wall in the way\n");
          
        }
      }else{
        teleporting = 0;
        if(hardnesspairAnotherVersion(d, next) == ter_wall_immutable){
          next[dim_x] -= c->position[dim_x];
          next[dim_y] -= c->position[dim_y];
          *error_message = const_cast<char*>("Cannot move there. That wall is immutable\n");
        }else{
          move_character(d, c, next, notifications);
        }
      }

      //Check if the oc is moving onto an object
      Objects* tmp = d->isObjecthere(c->position[dim_x], c->position[dim_y]);
      int carryCapacity = 10;
      if(tmp != nullptr){
        if(d->transfer_to_carry(tmp, carryCapacity)){
          //Item added
          *notifications = const_cast<char*>("Item Added \n");
          
        }else{
          //inventory full
          *notifications = const_cast<char*>("Inventory Full \n");

        }
      }


    //Move player
    //d->pc.position[dim_y] = next[dim_y];



    // //Setting players new position
    // d->pc.position[dim_y] = *pc_next[dim_y];
    // d->pc.position[dim_x] = *pc_next[dim_x];

    //printf("Player position updated: (%d, %d)\n", d->pc.position[dim_y], d->pc.position[dim_x]);

    d->render_player_version_dungeon(error_message, notifications);
    refresh();

    dijkstra(d);
    dijkstra_tunnel(d);
  }
}

void dir_nearest_wall(dungeon_t *d, character_t *c, pair_t dir)
{
  dir[dim_x] = dir[dim_y] = 0;

  if (c->position[dim_x] != 1 && c->position[dim_x] != DUNGEON_X - 2) {
    dir[dim_x] = (c->position[dim_x] > DUNGEON_X - c->position[dim_x] ? 1 : -1);
  }
  if (c->position[dim_y] != 1 && c->position[dim_y] != DUNGEON_Y - 2) {
    dir[dim_y] = (c->position[dim_y] > DUNGEON_Y - c->position[dim_y] ? 1 : -1);
  }
}

uint32_t against_wall(dungeon_t *d, character_t *c)
{
  return ((mapxy(d, c->position[dim_x] - 1,
                 c->position[dim_y]    ) == ter_wall_immutable) ||
          (mapxy(d, c->position[dim_x] + 1,
                 c->position[dim_y]    ) == ter_wall_immutable) ||
          (mapxy(d, c->position[dim_x]    ,
                 c->position[dim_y] - 1) == ter_wall_immutable) ||
          (mapxy(d, c->position[dim_x]    ,
                 c->position[dim_y] + 1) == ter_wall_immutable));
}

uint32_t in_corner(dungeon_t *d, character_t *c)
{
  uint32_t num_immutable;

  num_immutable = 0;

  num_immutable += (mapxy(d, c->position[dim_x] - 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(d, c->position[dim_x] + 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(d, c->position[dim_x]    ,
                          c->position[dim_y] - 1) == ter_wall_immutable);
  num_immutable += (mapxy(d, c->position[dim_x]    ,
                          c->position[dim_y] + 1) == ter_wall_immutable);

  return num_immutable > 1;
}
