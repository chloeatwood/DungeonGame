#include <stdlib.h>
#include <cstring>


#include "character.h"
#include "heap.h"
#include "npc.h"
#include "pc.h"
#include "dungeon.h"
#include "utils.h"
#include "event.h"
#include "path.h"



// int32_t character_t::compare_characters_by_next_turn(const character_t *other){

// }

bool try_displace_or_swap(dungeon_t *d, character_t *mover, pair_t next) {
  // Use int16_t for the grid coordinates to avoid narrowing issues
  int16_t y = next[dim_y], x = next[dim_x];

  // Ensure coordinates are within valid bounds
  if (y < 0 || y >= DUNGEON_Y || x < 0 || x >= DUNGEON_X) {
    return false; // Out of bounds, return early
  }

  character_t *occupant = d->character[y][x];

  if (!occupant || occupant->pc) return false; // Only handle NPC-vs-NPC

  // Try to displace occupant
  pair_t neighbors[8] = {
    {static_cast<int8_t>(y - 1), static_cast<int8_t>(x - 1)},
    {static_cast<int8_t>(y - 1), static_cast<int8_t>(x)},
    {static_cast<int8_t>(y - 1), static_cast<int8_t>(x + 1)},
    {static_cast<int8_t>(y), static_cast<int8_t>(x - 1)},
    {static_cast<int8_t>(y), static_cast<int8_t>(x + 1)},
    {static_cast<int8_t>(y + 1), static_cast<int8_t>(x - 1)},
    {static_cast<int8_t>(y + 1), static_cast<int8_t>(x)},
    {static_cast<int8_t>(y + 1), static_cast<int8_t>(x + 1)}
  };

  for (auto &n : neighbors) {
    int16_t ny = n[dim_y], nx = n[dim_x];

    // Ensure coordinates are within bounds and the terrain is not a wall
    if (nx < 0 || ny < 0 || nx >= DUNGEON_X || ny >= DUNGEON_Y) continue;

    if (!d->character[ny][nx] && d->map[ny][nx] != ter_wall) {
      // Move the occupant to the free cell
      d->character[ny][nx] = occupant;
      d->character[y][x] = mover;

      occupant->position[dim_y] = ny;
      occupant->position[dim_x] = nx;

      mover->position[dim_y] = y;
      mover->position[dim_x] = x;
      return true;
    }
  }

  // All neighboring cells occupied — perform a position swap
  pair_t temp;
  temp[dim_y] = mover->position[dim_y];
  temp[dim_x] = mover->position[dim_x];

  d->character[temp[dim_y]][temp[dim_x]] = occupant;
  d->character[y][x] = mover;

  occupant->position[dim_y] = temp[dim_y];
  occupant->position[dim_x] = temp[dim_x];

  mover->position[dim_y] = y;
  mover->position[dim_x] = x;
  return true;
}





void npc_next_pos_rand_tunnel(dungeon_t *d, character_t *c, pair_t next)
{
  pair_t n;
  union {
    uint32_t i;
    uint8_t a[4];
  } r;

  do {
    n[dim_y] = next[dim_y];
    n[dim_x] = next[dim_x];
    r.i = rand();
    if (r.a[0] > 85 /* 255 / 3 */) {
      if (r.a[0] & 1) {
        n[dim_y]--;
      } else {
        n[dim_y]++;
      }
    }
    if (r.a[1] > 85 /* 255 / 3 */) {
      if (r.a[1] & 1) {
        n[dim_x]--;
      } else {
        n[dim_x]++;
      }
    }
  } while (mappair(d, n) == ter_wall_immutable);

  if (hardnesspairAnotherVersion(d, n) <= 85) {
    if (hardnesspairAnotherVersion(d, n)) {
      hardnesspairAnotherVersion(d, n) = 0;
      mappair(d, n) = ter_floor_hall;

      /* Update distance maps because map has changed. */
      dijkstra(d);
      dijkstra_tunnel(d);
    }

    if (d->character[n[dim_y]][n[dim_x]]) {
      if (!try_displace_or_swap(d, c, n)) return; // Abort move if swap/displace fails
    }

    next[dim_x] = n[dim_x];
    next[dim_y] = n[dim_y];
  } else {
    hardnesspairAnotherVersion(d, n) -= 85;
  }
}

void npc_next_pos_rand(dungeon_t *d, character_t *c, pair_t next)
{
  pair_t n;
  union {
    uint32_t i;
    uint8_t a[4];
  } r;

  do {
    n[dim_y] = next[dim_y];
    n[dim_x] = next[dim_x];
    r.i = rand();
    if (r.a[0] > 85 /* 255 / 3 */) {
      if (r.a[0] & 1) {
        n[dim_y]--;
      } else {
        n[dim_y]++;
      }
    }
    if (r.a[1] > 85 /* 255 / 3 */) {
      if (r.a[1] & 1) {
        n[dim_x]--;
      } else {
        n[dim_x]++;
      }
    }
  } while (mappair(d, n) < ter_floor);

  if (d->character[n[dim_y]][n[dim_x]]) {
    if (!try_displace_or_swap(d, c, n)) return; // Abort move if swap/displace fails
  }

  next[dim_y] = n[dim_y];
  next[dim_x] = n[dim_x];
}

void npc_next_pos_line_of_sight(dungeon_t *d, character_t *c, pair_t next)
{
  pair_t dir;



  dir[dim_y] = d->pc->position[dim_y] - c->position[dim_y];
  dir[dim_x] = d->pc->position[dim_x] - c->position[dim_x];
  if (dir[dim_y]) {
    dir[dim_y] /= abs(dir[dim_y]);
  }
  if (dir[dim_x]) {
    dir[dim_x] /= abs(dir[dim_x]);
  }

  int new_y = next[dim_y] + dir[dim_y];
  int new_x = next[dim_x] + dir[dim_x];
  if (d->map[new_y][new_x] >= ter_floor) {
    if (d->character[new_y][new_x]) {
      if (!try_displace_or_swap(d, c, (pair_t){static_cast<int8_t>(std::min(std::max(new_y, -128), 127)), static_cast<int8_t>(std::min(std::max(new_x, -128), 127))})) return;
    }
    next[dim_x] = new_x;
    next[dim_y] = new_y;
  }
   else if (d->map[next[dim_y]][next[dim_x] + dir[dim_x]] >= ter_floor) {
    next[dim_x] += dir[dim_x];
  } else if (d->map[next[dim_y] + dir[dim_y]][next[dim_x]] >= ter_floor) {
    next[dim_y] += dir[dim_y];
  }
}

void npc_next_pos_line_of_sight_tunnel(dungeon_t *d,
                                       character_t *c,
                                       pair_t next)
{
  pair_t dir;

  dir[dim_y] = d->pc->position[dim_y] - c->position[dim_y];
  dir[dim_x] = d->pc->position[dim_x] - c->position[dim_x];
  if (dir[dim_y]) {
    dir[dim_y] /= abs(dir[dim_y]);
  }
  if (dir[dim_x]) {
    dir[dim_x] /= abs(dir[dim_x]);
  }

  dir[dim_x] += next[dim_x];
  dir[dim_y] += next[dim_y];

  if (hardnesspairAnotherVersion(d, dir) <= 85) {
     if (d->character[dir[dim_y]][dir[dim_x]]) {
      if (!try_displace_or_swap(d, c, dir)) return;
     }
    if (hardnesspairAnotherVersion(d, dir)) {
      hardnesspairAnotherVersion(d, dir) = 0;
      mappair(d, dir) = ter_floor_hall;

      /* Update distance maps because map has changed. */
      dijkstra(d);
      dijkstra_tunnel(d);
    }

    next[dim_x] = dir[dim_x];
    next[dim_y] = dir[dim_y];
  } else {
    hardnesspairAnotherVersion(d, dir) -= 85;
  }
}

void npc_next_pos_gradient(dungeon_t *d, character_t *c, pair_t next)
{
  /* Handles both tunneling and non-tunneling versions */
  pair_t min_next;
  uint16_t min_cost;
  if (c->npc->characteristics & NPC_TUNNEL) {
    min_cost = (d->pc_tunnel[next[dim_y] - 1][next[dim_x]] +
                (d->hardness[next[dim_y] - 1][next[dim_x]] / 85));
    min_next[dim_x] = next[dim_x];
    min_next[dim_y] = next[dim_y] - 1;
    if ((d->pc_tunnel[next[dim_y] + 1][next[dim_x]    ] +
         (d->hardness[next[dim_y] + 1][next[dim_x]] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y] + 1][next[dim_x]] +
                  (d->hardness[next[dim_y] + 1][next[dim_x]] / 85));
      min_next[dim_x] = next[dim_x];
      min_next[dim_y] = next[dim_y] + 1;
    }
    if ((d->pc_tunnel[next[dim_y]    ][next[dim_x] + 1] +
         (d->hardness[next[dim_y]    ][next[dim_x] + 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y]][next[dim_x] + 1] +
                  (d->hardness[next[dim_y]][next[dim_x] + 1] / 85));
      min_next[dim_x] = next[dim_x] + 1;
      min_next[dim_y] = next[dim_y];
    }
    if ((d->pc_tunnel[next[dim_y]    ][next[dim_x] - 1] +
         (d->hardness[next[dim_y]    ][next[dim_x] - 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y]][next[dim_x] - 1] +
                  (d->hardness[next[dim_y]][next[dim_x] - 1] / 85));
      min_next[dim_x] = next[dim_x] - 1;
      min_next[dim_y] = next[dim_y];
    }
    if ((d->pc_tunnel[next[dim_y] - 1][next[dim_x] + 1] +
         (d->hardness[next[dim_y] - 1][next[dim_x] + 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y] - 1][next[dim_x] + 1] +
                  (d->hardness[next[dim_y] - 1][next[dim_x] + 1] / 85));
      min_next[dim_x] = next[dim_x] + 1;
      min_next[dim_y] = next[dim_y] - 1;
    }
    if ((d->pc_tunnel[next[dim_y] + 1][next[dim_x] + 1] +
         (d->hardness[next[dim_y] + 1][next[dim_x] + 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y] + 1][next[dim_x] + 1] +
                  (d->hardness[next[dim_y] + 1][next[dim_x] + 1] / 85));
      min_next[dim_x] = next[dim_x] + 1;
      min_next[dim_y] = next[dim_y] + 1;
    }
    if ((d->pc_tunnel[next[dim_y] - 1][next[dim_x] - 1] +
         (d->hardness[next[dim_y] - 1][next[dim_x] - 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y] - 1][next[dim_x] - 1] +
                  (d->hardness[next[dim_y] - 1][next[dim_x] - 1] / 85));
      min_next[dim_x] = next[dim_x] - 1;
      min_next[dim_y] = next[dim_y] - 1;
    }
    if ((d->pc_tunnel[next[dim_y] + 1][next[dim_x] - 1] +
         (d->hardness[next[dim_y] + 1][next[dim_x] - 1] / 85)) < min_cost) {
      min_cost = (d->pc_tunnel[next[dim_y] + 1][next[dim_x] - 1] +
                  (d->hardness[next[dim_y] + 1][next[dim_x] - 1] / 85));
      min_next[dim_x] = next[dim_x] - 1;
      min_next[dim_y] = next[dim_y] + 1;
    }
    if (hardnesspairAnotherVersion(d, min_next) <= 85) {
      if (hardnesspairAnotherVersion(d, min_next)) {
        hardnesspairAnotherVersion(d, min_next) = 0;
        mappair(d, min_next) = ter_floor_hall;

        /* Update distance maps because map has changed. */
        dijkstra(d);
        dijkstra_tunnel(d);
      }

      if (d->character[min_next[dim_y]][min_next[dim_x]]) {
        if (!try_displace_or_swap(d, c, min_next)) return;
      }

      if (d->character[min_next[dim_y]][min_next[dim_x]]) {
        if (!try_displace_or_swap(d, c, min_next)) return;
      }

      next[dim_x] = min_next[dim_x];
      next[dim_y] = min_next[dim_y];
    } else {
      hardnesspairAnotherVersion(d, min_next) -= 85;
    }
  } else {
    /* Make monsters prefer cardinal directions */
    if (d->pc_distance[next[dim_y] - 1][next[dim_x]    ] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]--;
      return;
    }
    if (d->pc_distance[next[dim_y] + 1][next[dim_x]    ] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]++;
      return;
    }
    if (d->pc_distance[next[dim_y]    ][next[dim_x] + 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_x]++;
      return;
    }
    if (d->pc_distance[next[dim_y]    ][next[dim_x] - 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_x]--;
      return;
    }
    if (d->pc_distance[next[dim_y] - 1][next[dim_x] + 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]--;
      next[dim_x]++;
      return;
    }
    if (d->pc_distance[next[dim_y] + 1][next[dim_x] + 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]++;
      next[dim_x]++;
      return;
    }
    if (d->pc_distance[next[dim_y] - 1][next[dim_x] - 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]--;
      next[dim_x]--;
      return;
    }
    if (d->pc_distance[next[dim_y] + 1][next[dim_x] - 1] <
        d->pc_distance[next[dim_y]][next[dim_x]]) {
      next[dim_y]++;
      next[dim_x]--;
      return;
    }
  }
}

static void npc_next_pos_00(dungeon_t *d, character_t *c, pair_t next)
{
  /* not smart; not telepathic; not tunneling; not erratic */
  if (c->can_see(d, d->pc)) {
    c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
    c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
    npc_next_pos_line_of_sight(d, c, next);
  } else {
    npc_next_pos_rand(d, c, next);
  }
}

static void npc_next_pos_01(dungeon_t *d, character_t *c, pair_t next)
{
  /*     smart; not telepathic; not tunneling; not erratic */
  if (c->can_see(d, d->pc)) {
    c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
    c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
    c->npc->have_seen_pc = 1;
    npc_next_pos_line_of_sight(d, c, next);
  } else if (c->npc->have_seen_pc) {
    npc_next_pos_line_of_sight(d, c, next);
  }

  if (c->npc->have_seen_pc &&
      (next[dim_x] == c->npc->pc_last_known_position[dim_x]) &&
      (next[dim_y] == c->npc->pc_last_known_position[dim_y])) {
    c->npc->have_seen_pc = 0;
  }
}

static void npc_next_pos_02(dungeon_t *d, character_t *c, pair_t next)
{
  /* not smart;     telepathic; not tunneling; not erratic */
  c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
  c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
  npc_next_pos_line_of_sight(d, c, next);
}

static void npc_next_pos_03(dungeon_t *d, character_t *c, pair_t next)
{
  /*     smart;     telepathic; not tunneling; not erratic */
  npc_next_pos_gradient(d, c, next);
}

static void npc_next_pos_04(dungeon_t *d, character_t *c, pair_t next)
{
  /* not smart; not telepathic;     tunneling; not erratic */
  if (c->can_see(d, d->pc)) {
    c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
    c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
    npc_next_pos_line_of_sight(d, c, next);
  } else {
    npc_next_pos_rand_tunnel(d, c, next);
  }
}

static void npc_next_pos_05(dungeon_t *d, character_t *c, pair_t next)
{
  /*     smart; not telepathic;     tunneling; not erratic */
  if (c->can_see(d, d->pc)) {
    c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
    c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
    c->npc->have_seen_pc = 1;
    npc_next_pos_line_of_sight(d, c, next);
  } else if (c->npc->have_seen_pc) {
    npc_next_pos_line_of_sight_tunnel(d, c, next);
  }

  if (c->npc->have_seen_pc &&
      (next[dim_x] == c->npc->pc_last_known_position[dim_x]) &&
      (next[dim_y] == c->npc->pc_last_known_position[dim_y])) {
    c->npc->have_seen_pc = 0;
  }
}

static void npc_next_pos_06(dungeon_t *d, character_t *c, pair_t next)
{
  /* not smart;     telepathic;     tunneling; not erratic */
  c->npc->pc_last_known_position[dim_y] = d->pc->position[dim_y];
  c->npc->pc_last_known_position[dim_x] = d->pc->position[dim_x];
  npc_next_pos_line_of_sight_tunnel(d, c, next);
}

static void npc_next_pos_07(dungeon_t *d, character_t *c, pair_t next)
{
  /*     smart;     telepathic;     tunneling; not erratic */
  npc_next_pos_gradient(d, c, next);
}

static void npc_next_pos_erratic(dungeon_t *d, character_t *c, pair_t next);

void (*npc_move_func[])(dungeon_t *d, character_t *c, pair_t next) = {
  /* We'll have one function for each combination of bits, so the *
   * order is based on binary counting through the NPC_* bits.    *
   * It could be very easy to mess this up, so be careful.  We'll *
   * name them according to their hex value.                      */
  npc_next_pos_00,
  npc_next_pos_01,
  npc_next_pos_02,
  npc_next_pos_03,
  npc_next_pos_04,
  npc_next_pos_05,
  npc_next_pos_06,
  npc_next_pos_07,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
  npc_next_pos_erratic,
};

static void npc_next_pos_erratic(dungeon_t *d, character_t *c, pair_t next)
{
  /*                                               erratic */
  if (rand() & 1) {
    npc_next_pos_rand(d, c, next);
  } else {
    npc_move_func[c->npc->characteristics & 0x00000007](d, c, next);
  }
}

uint32_t character_t::dungeon_has_npcs(dungeon_t *d){
  return d->num_monsters;
}



uint32_t character_t::can_see(dungeon_t *d, character_t *other){
    pair_t first, second;
    pair_t del, f;
    int16_t a, b, c, i;

    first[dim_x] = this->position[dim_x];
    first[dim_y] = this->position[dim_y];
    second[dim_x] = other->position[dim_x];
    second[dim_y] = other->position[dim_y];

    if((abs(first[dim_x] - second[dim_x]) > VISUAL_RANGE) ||
        (abs(first[dim_y] - second[dim_y]) > VISUAL_RANGE)){
            return 0;
    }

    if (second[dim_x] > first[dim_x]) {
        del[dim_x] = second[dim_x] - first[dim_x];
        f[dim_x] = 1;
      } else {
        del[dim_x] = first[dim_x] - second[dim_x];
        f[dim_x] = -1;
      }
    
      if (second[dim_y] > first[dim_y]) {
        del[dim_y] = second[dim_y] - first[dim_y];
        f[dim_y] = 1;
      } else {
        del[dim_y] = first[dim_y] - second[dim_y];
        f[dim_y] = -1;
      }

      if (del[dim_x] > del[dim_y]) {
        a = del[dim_y] + del[dim_y];
        c = a - del[dim_x];
        b = c - del[dim_x];
        for (i = 0; i <= del[dim_x]; i++) {
          if ((mappair(d, first) < ter_floor) && i && (i != del[dim_x])) {
            return 0;
          }
          /*      mappair(first) = ter_debug;*/
          first[dim_x] += f[dim_x];
          if (c < 0) {
            c += a;
          } else {
            c += b;
            first[dim_y] += f[dim_y];
          }
        }
        return 1;
      } else {
        a = del[dim_x] + del[dim_x];
        c = a - del[dim_y];
        b = c - del[dim_y];
        for (i = 0; i <= del[dim_y]; i++) {
          if ((mappair(d, first) < ter_floor) && i && (i != del[dim_y])) {
            return 0;
          }
          /*      mappair(first) = ter_debug;*/
          first[dim_y] += f[dim_y];
          if (c < 0) {
            c += a;
          } else {
            c += b;
            first[dim_x] += f[dim_x];
          }
        }
        return 1;
      }
    
      return 1;
}

void character_t::character_delete(){
    //TODO: Fix after creating NPC class
    //this->npc->npc_delete();

  if (npc != nullptr) {
      delete npc;
  }
  if (pc != nullptr) {
      delete pc;
  }

    //free(this);

}


void character_t::npc_next_pos(dungeon_t *d, pair_t next){


    next[dim_y] = this->position[dim_y];
    next[dim_x] = this->position[dim_x];
  
    npc_move_func[this->npc->characteristics & 0x0000000f](d, this, next);
  
}

