#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "npc.h"
#include "dungeon.h"
#include "character.h"
#include "move.h"
#include "path.h"
#include "event.h"
#include "pc.h"
#include "dice.h"

/*Helper Functions for this class************************************************************************/

#define min(x, y) ({   \
  typeof (x) _x = (x); \
  typeof (y) _y = (y); \
  _x < _y ? _x : _y;   \
})



/*Main functions in this class **********************************************************************************/


void npc_t::npc_delete(){
    free(this);
}

void npc_t::setNAME(std::string N){
  NAME = N;
}
void npc_t::setDESC(std::string D){
  DESC = D;
}
void npc_t::setCOLOR(std::string C){
  COLOR = C;
}
void npc_t::setSPEED(std::vector<char> S){
  //setting character_t speed
  this->speed = parseDice(S).rollDaDice();
}
void npc_t::setABIL(std::vector<std::string> A){
  //paarse through abilities to set
  ABIL = A;
  characteristics = 0;

  for (const std::string &abil : A) {
      if (abil == "SMART") {
          characteristics |= NPC_SMART;
      } else if (abil == "TELE") {
          characteristics |= NPC_TELEPATH;
      } else if (abil == "TUNNEL") {
          characteristics |= NPC_TUNNEL;
      } else if (abil == "ERRATIC") {
          characteristics |= NPC_ERRATIC;
      } else if (abil == "UNIQ") {
          characteristics |= NPC_BIT04;
          Unique = true;
      } else if (abil == "DESTROY") {
          //characteristics |= NPC_BIT05;
      } else if (abil == "PASS") {
          //characteristics |= NPC_BIT06;
      } else if (abil == "PICKUP") {
          //characteristics |= NPC_BIT07;
      } else if (abil == "BOSS") {
          characteristics |= NPC_BIT08;
          BOSS = 1;
      } else if (abil == "BIT09") {
          //characteristics |= NPC_BIT09;
      } else if (abil == "BIT10") {
          //characteristics |= NPC_BIT10;
      }
    //call setBOSS
    if(BOSS != 1){
      setBoss(0);
    }
  }
}
void npc_t::setHP(std::vector<char> H){
  this->HP = parseDice(H).rollDaDice();
}
void npc_t::setDAM(std::vector<char> DA){
  this->DAM = DA;
}
void npc_t::setSYMB(char SYM){
  SYMB = SYM;
}
void npc_t::setRRTY(float RR){
  RRTY = RR;
}
void npc_t::setBoss(int b){
  BOSS = b;
}
int npc_t::isBoss(){
  return BOSS;
}

bool npc_t::isUnique(){
  return Unique;
}



