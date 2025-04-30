#include <stdlib.h>
#include <ncurses.h>
#include <sstream>


#include "string.h"

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"
#include "npc.h"

//Delete the pc
void pc_t::pc_delete(){
  // if (this->pc) {
  //   free(this->pc);
  // }
  // free(this);

  delete this;
}

uint32_t pc_t::pc_is_alive(dungeon_t *d){
    return d->pc->alive;
}

void pc_t::config_pc(dungeon_t *d){

    // m = static_cast<character_t*>(malloc(sizeof(character_t)));  // Allocate memory
    d->pc = new pc_t();  // Allocate and construct

    // memset(&d->pc, 0, sizeof (d->pc));
    d->pc->symbol = '@';
  
    place_pc(d);
  
    d->pc->speed = PC_SPEED;
    d->pc->alive = 1;
    d->pc->sequence_number = 0;
    d->pc->pc = (pc_t*)calloc(1, sizeof (*d->pc->pc));
    d->pc->npc = NULL;
    d->pc->kills[kill_direct] = d->pc->kills[kill_avenged] = 0;
    d->quit = 1;
  
    d->character[d->pc->position[dim_y]][d->pc->position[dim_x]] = d->pc;
  
    dijkstra(d);
    dijkstra_tunnel(d);  
}

void undo_move(int keyPressed, pair_t dir, character_t *tmp){

  switch(keyPressed){
    //Go to the Upper-Left Corner of current position
    case 'y': case '7':
      dir[dim_y] += 2;
      dir[dim_x] += 2;
      tmp->position[dim_y] += 2;
      tmp->position[dim_x] += 2;
      break;
    //Go Up from current position
    case '8': case 'k':
      dir[dim_y] += 2;
      tmp->position[dim_y] += 2;
      break;
    //Go Upper-Right Corner of current position
    case '9': case 'u':
      dir[dim_y] += 2;
      dir[dim_x] -= 2;
      tmp->position[dim_y] += 2;
      tmp->position[dim_x] -= 2;
      break;
    //Move right 
    case '6': case 'l':
      dir[dim_x] -= 2;
      tmp->position[dim_x] -= 2;
      break;
    //Move down-right
    case '3': case 'n':
      dir[dim_y] -= 2;
      dir [dim_x] -= 2;
      tmp->position[dim_y] -= 2;
      tmp->position[dim_x] -= 2;
      break;
    //move down
    case '2': case 'j':
      dir[dim_y] -= 2;
      tmp->position[dim_y] -= 2;
      break;
    //Move down-left
    case '1': case 'b':
      dir[dim_y] -= 2;
      dir[dim_x] += 2;
      tmp->position[dim_y] -= 2;
      tmp->position[dim_x] += 2;
      break;
    //Move left
    case '4': case 'h':
      dir[dim_x] += 2;
      tmp->position[dim_x] += 2;
      break;
  }
}

uint32_t pc_t::teleporting_mode(dungeon_t *d, pair_t dir, char **error_message){

  dir[dim_y] = dir[dim_x] = 0;
  //Replace pc with targeting cursor
  character_t *tmp;
  tmp = new character_t();
  tmp->position[dim_y] = d->pc->position[dim_y];
  tmp->position[dim_x] = d->pc->position[dim_x];
  //d->character[tmp->position[dim_y]][tmp->position[dim_x]] = tmp;
  //tmp->speed = PC_SPEED;
  tmp->alive = 1;
  //m->sequence_number
  //m->pc = NULL;
  tmp->symbol = '+';
  //Pull up map

  d->render_dungeon(error_message);
  mvaddch(tmp->position[dim_y] + 3, tmp->position[dim_x], '+');

  int keyPressed = getch();

  while(keyPressed != 'g' && keyPressed != 'r'){
    if(tmp->position[dim_y] < DUNGEON_Y && tmp->position[dim_x] < DUNGEON_X && tmp->position[dim_y] > 0 && tmp->position[dim_x] > 0){

      switch(keyPressed){
        //Go to the Upper-Left Corner of current position
        case 'y': case '7':
          dir[dim_y] -= 1;
          dir[dim_x] -= 1;
          tmp->position[dim_y]--;
          tmp->position[dim_x]--;
          break;
        //Go Up from current position
        case '8': case 'k':
          dir[dim_y] -= 1;
          tmp->position[dim_y]--;
          break;
        //Go Upper-Right Corner of current position
        case '9': case 'u':
          dir[dim_y] -= 1;
          dir[dim_x] += 1;
          tmp->position[dim_y]--;
          tmp->position[dim_x]++;
          break;
        //Move right 
        case '6': case 'l':
          dir[dim_x] += 1;
          tmp->position[dim_x]++;
          break;
        //Move down-right
        case '3': case 'n':
          dir[dim_y] += 1;
          dir [dim_x] += 1;
          tmp->position[dim_y]++;
          tmp->position[dim_x]++;
          break;
        //move down
        case '2': case 'j':
          dir[dim_y] += 1;
          tmp->position[dim_y]++;
          break;
        //Move down-left
        case '1': case 'b':
          dir[dim_y] += 1;
          dir[dim_x] -= 1;
          tmp->position[dim_y]++;
          tmp->position[dim_x]--;
          break;
        //Move left
        case '4': case 'h':
          dir[dim_x] -= 1;
          tmp->position[dim_x]--;
          break;
        //Rest for a turn 
        case '5': case '.': case ' ':
          //Nothing happens, Resting
          break;
        case 'g':
          //Exiting teleporting mode. Should not make it here
          break;
      }

        d->render_dungeon(error_message);
        *error_message = const_cast<char*>("\n");
        mvaddch(tmp->position[dim_y] + 3, tmp->position[dim_x], '+');
        keyPressed = getch();


    }else{
      *error_message = const_cast<char*>("Cannot move there. That is immutable rock. Undoing last move\n");
      undo_move(keyPressed, dir, tmp);
      d->render_dungeon(error_message);

    }
  }

  if(keyPressed == 'r'){
    int newX = rand() % 79 + 1;
    int newY = rand() % 20 + 1;

    while(mapxy(d, newX, newY) == ter_wall || mapxy(d, newX, newY) == ter_wall_immutable){
      newX = rand() % 79 + 1;
      newY = rand() % 20 + 1;
    }

    dir[dim_y] = dir[dim_x] = 0;

    d->pc->position[dim_x] = newX;
    d->pc->position[dim_y] = newY;
  }

  //Kill the tmp character. delete
  //delete tmp;
  return 0;
}

bool within_radius(pair_t pc_pos, pair_t cursor_pos) {
  return abs(pc_pos[dim_x] - cursor_pos[dim_x]) <= 2 &&
         abs(pc_pos[dim_y] - cursor_pos[dim_y]) <= 2;
}


uint32_t pc_t::inspect_mode(dungeon_t *d, pair_t dir, char **error_message, char **notifications) {
  dir[dim_y] = dir[dim_x] = 0;

  character_t *cursor = new character_t();
  cursor->position[dim_y] = d->pc->position[dim_y];
  cursor->position[dim_x] = d->pc->position[dim_x];
  cursor->symbol = '+';
  cursor->alive = 1;

  *notifications = const_cast<char*>("Press q to exit");


  d->render_player_version_dungeon(error_message, notifications);
  mvaddch(cursor->position[dim_y] + 3, cursor->position[dim_x], '+');
  int keyPressed = getch();

  while (keyPressed != 'q') {  // 'q' to quit inspect mode
    if (cursor->position[dim_y] < DUNGEON_Y && cursor->position[dim_x] < DUNGEON_X &&
        cursor->position[dim_y] > 0 && cursor->position[dim_x] > 0) {
  
      // Reset next_pos to current cursor position
      pair_t next_pos;
      next_pos[dim_x] = cursor->position[dim_x];
      next_pos[dim_y] = cursor->position[dim_y];
  
      switch (keyPressed) {
        case 'y': case '7': next_pos[dim_y]--; next_pos[dim_x]--; break;
        case '8': case 'k': next_pos[dim_y]--; break;
        case '9': case 'u': next_pos[dim_y]--; next_pos[dim_x]++; break;
        case '6': case 'l': next_pos[dim_x]++; break;
        case '3': case 'n': next_pos[dim_y]++; next_pos[dim_x]++; break;
        case '2': case 'j': next_pos[dim_y]++; break;
        case '1': case 'b': next_pos[dim_y]++; next_pos[dim_x]--; break;
        case '4': case 'h': next_pos[dim_x]--; break;
        case '5': case '.': case ' ': break;  // Rest
        case 'i': {
          character_t *target = d->character[cursor->position[dim_y]][cursor->position[dim_x]];
          if (target && !target->pc) {
            npc_t *npc = target->npc;
            std::vector<std::string> info;
            info.push_back("Monster Info:");
            info.push_back("Name: " + std::string(npc->NAME));
            info.push_back("Speed: " + std::to_string(npc->speed));
            info.push_back("HP: " + std::to_string(npc->HP));
            info.push_back("Damage: " + std::string(npc->DAM.begin(), npc->DAM.end()) +
                           " (avg ~" + std::to_string(parseDice(npc->DAM).rollDaDice()) + ")");
            info.push_back("Description:");
            
            std::istringstream desc_stream(npc->DESC);
            std::string line;
            while (std::getline(desc_stream, line)) {
              info.push_back(line);
            }
        
            d->display_message_popup(info, "Inspecting Monster");
            *notifications = const_cast<char*>("Press q to exit");
          } else {
            std::vector<std::string> info = {"No monster at this position."};
            d->display_message_popup(info, "Inspecting Monster");
            *notifications = const_cast<char*>("Press q to exit");
          }
          break;
        }
        case 'K': {
          character_t *target = d->character[cursor->position[dim_y]][cursor->position[dim_x]];
          if (target && !target->pc && target->alive) {
            target->alive = 0;

            if (target != d->pc) {
              d->num_monsters--;
              if (target->npc->isUnique() == 1) {
                d->extinct_monsters->push_back(target->npc);
              }
            }

            if (target->npc) {
              if (target->npc->isBoss()) {
                d->pc->WIN = 1;
                std::string msg = "Boss slain instantly! You win. WIN = " + std::to_string(d->pc->WIN);
                *notifications = strdup(msg.c_str());
              }

              target->npc = nullptr;  // Free the NPC info if needed
            }

            *notifications = const_cast<char*>("Monster instantly slain.");
          } else {
            *error_message = const_cast<char*>("No living monster here to kill.\n");
          }
          break;
        }
        
      }
  
      // Apply movement only if it's in-bounds and within radius
      if (next_pos[dim_x] > 0 && next_pos[dim_x] < DUNGEON_X &&
          next_pos[dim_y] > 0 && next_pos[dim_y] < DUNGEON_Y &&
          within_radius(d->pc->position, next_pos)) {
        cursor->position[dim_x] = next_pos[dim_x];
        cursor->position[dim_y] = next_pos[dim_y];
      } else {
        *error_message = const_cast<char*>("Can't move cursor that far from player.\n");
      }
  
      d->render_player_version_dungeon(error_message, notifications);
      mvaddch(cursor->position[dim_y] + 3, cursor->position[dim_x], '+');
      keyPressed = getch();
    } else {
      *error_message = const_cast<char*>("Out of bounds. Undoing last move.\n");
      undo_move(keyPressed, dir, cursor);
      d->render_player_version_dungeon(error_message, notifications);
      mvaddch(cursor->position[dim_y] + 3, cursor->position[dim_x], '+');
      keyPressed = getch();
    }
  }
  

  delete cursor;
  return 0;
}



uint32_t pc_t::pc_next_pos(dungeon_t *d, pair_t dir, int keyPressed, char ** error_message, int *teleporting, char **notifications){
    dir[dim_y] = dir[dim_x] = 0;

    switch(keyPressed){
      //Go to the Upper-Left Corner of current position
      case 'y': case '7':
        dir[dim_y] -= 1;
        dir[dim_x] -= 1;
        break;
      //Go Up from current position
      case '8': case 'k':
        dir[dim_y] -= 1;
        break;
      //Go Upper-Right Corner of current position
      case '9': case 'u':
        dir[dim_y] -= 1;
        dir[dim_x] += 1;
        break;
      //Move right 
      case '6': case 'l':
        dir[dim_x] += 1;
        break;
      //Move down-right
      case '3': case 'n':
        dir[dim_y] += 1;
        dir [dim_x] += 1;
        break;
      //move down
      case '2': case 'j':
        dir[dim_y] += 1;
        break;
      //Move down-left
      case '1': case 'b':
        dir[dim_y] += 1;
        dir[dim_x] -= 1;
        break;
      //Move left
      case '4': case 'h':
        dir[dim_x] -= 1;
        break;
      //Rest for a turn 
      case '5': case '.': case ' ':
        //Nothing happens, Resting
        break;
      case '<': case '>':
      //*error_message = "You Reached Stairs\n";
        //If the current player position is < || >
        if(mappair(d, d->pc->position) == ter_stairs_down || mappair(d, d->pc->position) == ter_stairs_up || mappair(d, d->pc->position) == ter_stairs){
          //*error_message = "You Reached Stairs\n";
          //endwin();
          // Debugging before reset
          // Clear input buffer
          flushinp();  
  
          // Debugging before reset
          mvprintw(0, 0, "Before Reset: PC at (%d, %d)", 
                   d->pc->position[dim_x], d->pc->position[dim_y]);
  
          // Reset dungeon
          d->clear_dungeon_for_reset();
          d->init_dungeon();
          d->gen_dungeon();
          //config_pc(d);
          place_pc(d);
          d->gen_monsters();
          d->gen_objects();
  
          // Debugging after reset
          mvprintw(1, 0, "After Reset: PC at (%d, %d)", 
                   d->pc->position[dim_x], d->pc->position[dim_y]);
  
          cbreak();
          noecho();
          keypad(stdscr, TRUE);
          
          d->render_player_version_dungeon(error_message, notifications);
          refresh();
  
        }
        break;
      case 'g':
        //Enter teleporting mode function
        *teleporting = 1;
        teleporting_mode(d, dir, error_message);
        break;

      case 'L':
        inspect_mode(d, dir, error_message, notifications);
        break;
    }
  
    // // Draw initial position
    // mvprintw(dir[dim_y], dir[dim_x], "@");
    // refresh();
  
  
    //return 0 since the info that will be used from this is the dir[x]. dir[y] for next direction
    return 0; 
}

void pc_t::place_pc(dungeon_t *d){
    d->pc->position[dim_y] = rand_range(d->rooms->position[dim_y],
        (d->rooms->position[dim_y] +
         d->rooms->size[dim_y] - 1));
    d->pc->position[dim_x] = rand_range(d->rooms->position[dim_x],
        (d->rooms->position[dim_x] +
         d->rooms->size[dim_x] - 1)); 
}

uint32_t pc_t::pc_in_room(dungeon_t *d, uint32_t room){
    if((room < d->num_rooms) && (d->pc->position[dim_x] >= d->rooms[room].position[dim_x]) &&
    (d->pc->position[dim_x] < (d->rooms[room].position[dim_x] + d->rooms[room].size[dim_x])) &&
    (d->pc->position[dim_y] >= d->rooms[room].position[dim_y]) && (d->pc->position[dim_y] < (d->rooms[room].position[dim_y] +
    d->rooms[room].size[dim_y]))){
        return 1;
    }
    return 0;
}

uint32_t pc_t::pc_quit(dungeon_t *d){
    return d->pc->quit;
}

