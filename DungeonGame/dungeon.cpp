#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <ncurses.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <filesystem>
#include <vector>
#include <sstream>
#include <sstream> 
#include <unordered_map>


#include "heap.h"
#include "dungeon.h"
#include "utils.h"
#include "event.h"
#include "npc.h"

#define DUMP_HARDNESS_IMAGES 0

typedef struct corridor_path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} corridor_path_t;


/*Helper functions **********************************************************************************/

static uint32_t adjacent_to_room(dungeon_t *d, int16_t y, int16_t x)
{
  return (mapxy(d, x - 1, y) == ter_floor_room ||
          mapxy(d, x + 1, y) == ter_floor_room ||
          mapxy(d, x, y - 1) == ter_floor_room ||
          mapxy(d, x, y + 1) == ter_floor_room);
}

static uint32_t is_open_space(dungeon_t *d, int16_t y, int16_t x)
{
  return !hardnessxy(d, x, y);
}

static int32_t corridor_path_cmp(const void *key, const void *with) {
  return ((corridor_path_t *) key)->cost - ((corridor_path_t *) with)->cost;
}

static void dijkstra_corridor(dungeon_t *d, pair_t from, pair_t to)
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(d, x, y) != ter_wall_immutable) {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      } else {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = (corridor_path_t*)heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != (uint32_t)from[dim_x]) || (y != (uint32_t)from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        if (d->map[y][x] != ter_floor_room) {
          d->map[y][x] = ter_floor_hall;
          d->hardness[y][x] = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair(d, p->pos))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair(d, p->pos);
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         p->cost + hardnesspair(d, p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost =
        p->cost + hardnesspair(d, p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         p->cost + hardnesspair(d, p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost =
        p->cost + hardnesspair(d, p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair(d, p->pos))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair(d, p->pos);
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

static inline uint8_t hardnesspair_inv(dungeon_t *d, const uint8_t (&p)[2]) {
  if (is_open_space(d, p[dim_y], p[dim_x])) {
    return 127;
  }
  if (adjacent_to_room(d, p[dim_y], p[dim_x])) {
    return 191;
  }
  return 255 - hardnesspair(d, p); // Works with uint8_t
}

/* This is a cut-and-paste of the above.  The code is modified to  *
 * calculate paths based on inverse hardnesses so that we get a    *
 * high probability of creating at least one cycle in the dungeon. */
static void dijkstra_corridor_inv(dungeon_t *d, pair_t from, pair_t to)
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (d->map[y][x] != ter_wall_immutable) {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      } else {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = (corridor_path_t*)heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != (uint32_t)from[dim_x]) || (y != (uint32_t)from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        if (d->map[y][x] != ter_floor_room) {
          d->map[y][x] = ter_floor_hall;
          //hardnessxy(x, y) = 0;
          d->hardness[y][x] = 0;
        }
      }
      heap_delete(&h);
      return;
    }


    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair_inv(d, p->pos))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair_inv(d, p->pos);
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         p->cost + hardnesspair_inv(d, p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost =
        p->cost + hardnesspair_inv(d, p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         p->cost + hardnesspair_inv(d, p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost =
        p->cost + hardnesspair_inv(d, p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair_inv(d, p->pos))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair_inv(d, p->pos);
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

/* Chooses a random point inside each room and connects them with a *
 * corridor.  Random internal points prevent corridors from exiting *
 * rooms in predictable locations.                                  */
static int connect_two_rooms(dungeon_t *d, room_t *r1, room_t *r2)
{
  pair_t e1, e2;

  e1[dim_y] = rand_range(r1->position[dim_y],
                         r1->position[dim_y] + r1->size[dim_y] - 1);
  e1[dim_x] = rand_range(r1->position[dim_x],
                         r1->position[dim_x] + r1->size[dim_x] - 1);
  e2[dim_y] = rand_range(r2->position[dim_y],
                         r2->position[dim_y] + r2->size[dim_y] - 1);
  e2[dim_x] = rand_range(r2->position[dim_x],
                         r2->position[dim_x] + r2->size[dim_x] - 1);

  /*  return connect_two_points_recursive(d, e1, e2);*/
  dijkstra_corridor(d, e1, e2);

  return 0;
}

static int create_cycle(dungeon_t *d)
{
  /* Find the (approximately) farthest two rooms, then connect *
   * them by the shortest path using inverted hardnesses.      */

  int32_t max, tmp, i, j, p, q;
  pair_t e1, e2;

  for (i = max = 0; i < (int)d->num_rooms - 1; i++) {
    for (j = i + 1; j < (int)d->num_rooms; j++) {
      tmp = (((d->rooms[i].position[dim_x] - d->rooms[j].position[dim_x])  *
              (d->rooms[i].position[dim_x] - d->rooms[j].position[dim_x])) +
             ((d->rooms[i].position[dim_y] - d->rooms[j].position[dim_y])  *
              (d->rooms[i].position[dim_y] - d->rooms[j].position[dim_y])));
      if (tmp > max) {
        max = tmp;
        p = i;
        q = j;
      }
    }
  }

  /* Can't simply call connect_two_rooms() because it doesn't *
   * use inverse hardnesses, so duplicate it here.            */
  e1[dim_y] = rand_range(d->rooms[p].position[dim_y],
                         (d->rooms[p].position[dim_y] +
                          d->rooms[p].size[dim_y] - 1));
  e1[dim_x] = rand_range(d->rooms[p].position[dim_x],
                         (d->rooms[p].position[dim_x] +
                          d->rooms[p].size[dim_x] - 1));
  e2[dim_y] = rand_range(d->rooms[q].position[dim_y],
                         (d->rooms[q].position[dim_y] +
                          d->rooms[q].size[dim_y] - 1));
  e2[dim_x] = rand_range(d->rooms[q].position[dim_x],
                         (d->rooms[q].position[dim_x] +
                          d->rooms[q].size[dim_x] - 1));

  dijkstra_corridor_inv(d, e1, e2);

  return 0;
}

static int connect_rooms(dungeon_t *d)
{
  uint32_t i;

  for (i = 1; i < d->num_rooms; i++) {
    connect_two_rooms(d, d->rooms + i - 1, d->rooms + i);
  }

  create_cycle(d);

  return 0;
}

int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int smooth_hardness(dungeon_t *d)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
#if DUMP_HARDNESS_IMAGES
  FILE *out;
#endif
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];

  memset(&hardness, 0, sizeof (hardness));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % DUNGEON_X;
      y = rand() % DUNGEON_Y;
    } while (hardness[y][x]);
    hardness[y][x] = i;
    if (i == 1) {
      head = tail = (queue_node_t*)malloc(sizeof (*tail));
    } else {
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

#if DUMP_HARDNESS_IMAGES
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&hardness, sizeof (hardness), 1, out);
  fclose(out);
#endif
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = hardness[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !hardness[y - 1][x - 1]) {
      hardness[y - 1][x - 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !hardness[y][x - 1]) {
      hardness[y][x - 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < DUNGEON_Y && !hardness[y + 1][x - 1]) {
      hardness[y + 1][x - 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !hardness[y - 1][x]) {
      hardness[y - 1][x] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < DUNGEON_Y && !hardness[y + 1][x]) {
      hardness[y + 1][x] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < DUNGEON_X && y - 1 >= 0 && !hardness[y - 1][x + 1]) {
      hardness[y - 1][x + 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < DUNGEON_X && !hardness[y][x + 1]) {
      hardness[y][x + 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < DUNGEON_X && y + 1 < DUNGEON_Y && !hardness[y + 1][x + 1]) {
      hardness[y + 1][x + 1] = i;
      tail->next = (queue_node_t*)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < DUNGEON_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < DUNGEON_X) {
            s += gaussian[p][q];
            t += hardness[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      d->hardness[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < DUNGEON_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < DUNGEON_X) {
            s += gaussian[p][q];
            t += hardness[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      d->hardness[y][x] = t / s;
    }
  }

#if DUMP_HARDNESS_IMAGES
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&hardness, sizeof (hardness), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&d->hardness, sizeof (d->hardness), 1, out);
  fclose(out);
#endif

  return 0;
}

static int empty_dungeon(dungeon_t *d)
{
  uint8_t x, y;

  smooth_hardness(d);
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      d->map[y][x] = ter_wall;
      if (y == 0 || y == DUNGEON_Y - 1 ||
          x == 0 || x == DUNGEON_X - 1) {
        d->map[y][x] = ter_wall_immutable;
        d->hardness[y][x] = 255;
      }
    }
  }

  return 0;
}

static int place_rooms(dungeon_t *d)
{
  pair_t p;
  uint32_t i;
  int success;
  room_t *r;

  for (success = 0; !success; ) {
    success = 1;
    for (i = 0; success && i < d->num_rooms; i++) {
      r = d->rooms + i;
      r->position[dim_x] = 1 + rand() % (DUNGEON_X - 2 - r->size[dim_x]);
      r->position[dim_y] = 1 + rand() % (DUNGEON_Y - 2 - r->size[dim_y]);
      for (p[dim_y] = r->position[dim_y] - 1;
           success && p[dim_y] < r->position[dim_y] + r->size[dim_y] + 1;
           p[dim_y]++) {
        for (p[dim_x] = r->position[dim_x] - 1;
             success && p[dim_x] < r->position[dim_x] + r->size[dim_x] + 1;
             p[dim_x]++) {
          if (mappair(d, p) >= ter_floor) {
            success = 0;
            empty_dungeon(d);
          } else if ((p[dim_y] != r->position[dim_y] - 1)              &&
                     (p[dim_y] != r->position[dim_y] + r->size[dim_y]) &&
                     (p[dim_x] != r->position[dim_x] - 1)              &&
                     (p[dim_x] != r->position[dim_x] + r->size[dim_x])) {
            mappair(d, p) = ter_floor_room;
            hardnesspairAnotherVersion(d, p) = 0;
          }
        }
      }
    }
  }

  return 0;
}

static void place_stairs(dungeon_t *d)
{
  pair_t p;
  do {
    while ((p[dim_y] = rand_range(1, DUNGEON_Y - 2)) &&
           (p[dim_x] = rand_range(1, DUNGEON_X - 2)) &&
           ((mappair(d, p) < ter_floor)                 ||
            (mappair(d, p) > ter_stairs)))
      ;
    mappair(d, p) = ter_stairs_down;
  } while (rand_under(1, 3));
  do {
    while ((p[dim_y] = rand_range(1, DUNGEON_Y - 2)) &&
           (p[dim_x] = rand_range(1, DUNGEON_X - 2)) &&
           ((mappair(d, p) < ter_floor)                 ||
            (mappair(d, p) > ter_stairs)))
      
      ;
    mappair(d, p) = ter_stairs_up;
  } while (rand_under(2, 4));
}

static int make_rooms(dungeon_t *d)
{
  uint32_t i;

  for (i = MIN_ROOMS; i < MAX_ROOMS && rand_under(5, 8); i++)
    ;
  d->num_rooms = i;
  d->rooms = (room_t*)malloc(sizeof (*d->rooms) * d->num_rooms);
  
  for (i = 0; i < d->num_rooms; i++) {
    d->rooms[i].size[dim_x] = ROOM_MIN_X;
    d->rooms[i].size[dim_y] = ROOM_MIN_Y;
    while (rand_under(3, 5) && d->rooms[i].size[dim_x] < ROOM_MAX_X) {
      d->rooms[i].size[dim_x]++;
    }
    while (rand_under(3, 5) && d->rooms[i].size[dim_y] < ROOM_MAX_Y) {
      d->rooms[i].size[dim_y]++;
    }
  }

  return 0;
}

int write_dungeon_map(dungeon_t *d, FILE *f)
{
  uint32_t x, y;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      fwrite(&d->hardness[y][x], sizeof (unsigned char), 1, f);
    }
  }

  return 0;
}

int write_rooms(dungeon_t *d, FILE *f)
{
  uint32_t i;
  uint16_t p;

  p = htobe16(d->num_rooms);
  fwrite(&p, 2, 1, f);
  for (i = 0; i < d->num_rooms; i++) {
    /* write order is xpos, ypos, width, height */
    p = d->rooms[i].position[dim_x];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].position[dim_y];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].size[dim_x];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].size[dim_y];
    fwrite(&p, 1, 1, f);
  }

  return 0;
}

uint16_t count_up_stairs(dungeon_t *d)
{
  uint32_t x, y;
  uint16_t i;

  for (i = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->map[y][x] == ter_stairs_up) {
        i++;
      }
    }
  }

  return i;
}

uint16_t count_down_stairs(dungeon_t *d)
{
  uint32_t x, y;
  uint16_t i;

  for (i = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->map[y][x] == ter_stairs_down) {
        i++;
      }
    }
  }

  return i;
}

int write_stairs(dungeon_t *d, FILE *f)
{
  uint16_t num_stairs;
  uint8_t x, y;

  num_stairs = htobe16(count_up_stairs(d));
  fwrite(&num_stairs, 2, 1, f);
  for (y = 1; y < DUNGEON_Y - 1 && num_stairs; y++) {
    for (x = 1; x < DUNGEON_X - 1 && num_stairs; x++) {
      if (d->map[y][x] == ter_stairs_up) {
        num_stairs--;
        fwrite(&x, 1, 1, f);
        fwrite(&y, 1, 1, f);
      }
    }
  }

  num_stairs = htobe16(count_down_stairs(d));
  fwrite(&num_stairs, 2, 1, f);
  for (y = 1; y < DUNGEON_Y - 1 && num_stairs; y++) {
    for (x = 1; x < DUNGEON_X - 1 && num_stairs; x++) {
      if (d->map[y][x] == ter_stairs_down) {
        num_stairs--;
        fwrite(&x, 1, 1, f);
        fwrite(&y, 1, 1, f);
      }
    }
  }

  return 0;
}

uint32_t calculate_dungeon_size(dungeon_t *d)
{
  /* Per the spec, 1708 is 12 byte semantic marker + 4 byte file verion + *
   * 4 byte file size + 2 byte PC position + 1680 byte hardness array +   *
   * 2 byte each number of rooms, number of up stairs, number of down     *
   * stairs.                                                              */
  return (1708 + (d->num_rooms * 4) +
          (count_up_stairs(d) * 2)  +
          (count_down_stairs(d) * 2));
}

int read_dungeon_map(dungeon_t *d, FILE *f)
{
  uint32_t x, y;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      fread(&d->hardness[y][x], sizeof (d->hardness[y][x]), 1, f);
      if (d->hardness[y][x] == 0) {
        /* Mark it as a corridor.  We can't recognize room cells until *
         * after we've read the room array, which we haven't done yet. */
        d->map[y][x] = ter_floor_hall;
      } else if (d->hardness[y][x] == 255) {
        d->map[y][x] = ter_wall_immutable;
      } else {
        d->map[y][x] = ter_wall;
      }
    }
  }


  return 0;
}

int read_stairs(dungeon_t *d, FILE *f)
{
  uint16_t num_stairs;
  uint8_t x, y;

  fread(&num_stairs, 2, 1, f);
  num_stairs = be16toh(num_stairs);
  for (; num_stairs; num_stairs--) {
    fread(&x, 1, 1, f);
    fread(&y, 1, 1, f);
    d->map[y][x] = ter_stairs_up;
  }

  fread(&num_stairs, 2, 1, f);
  num_stairs = be16toh(num_stairs);
  for (; num_stairs; num_stairs--) {
    fread(&x, 1, 1, f);
    fread(&y, 1, 1, f);
    d->map[y][x] = ter_stairs_down;
  }
  return 0;
}

int read_rooms(dungeon_t *d, FILE *f)
{
  uint32_t i;
  uint32_t x, y;
  uint16_t p;

  fread(&p, 2, 1, f);
  d->num_rooms = be16toh(p);
  d->rooms = (room_t*)malloc(sizeof (*d->rooms) * d->num_rooms);

  for (i = 0; i < d->num_rooms; i++) {
    fread(&p, 1, 1, f);
    d->rooms[i].position[dim_x] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].position[dim_y] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].size[dim_x] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].size[dim_y] = p;

    if (d->rooms[i].size[dim_x] < 1             ||
        d->rooms[i].size[dim_y] < 1             ||
        d->rooms[i].size[dim_x] > DUNGEON_X - 1 ||
        d->rooms[i].size[dim_y] > DUNGEON_Y - 1) {
      fprintf(stderr, "Invalid room size in restored dungeon.\n");

      exit(-1);
    }

    if (d->rooms[i].position[dim_x] < 1                                       ||
        d->rooms[i].position[dim_y] < 1                                       ||
        d->rooms[i].position[dim_x] > DUNGEON_X - 1                           ||
        d->rooms[i].position[dim_y] > DUNGEON_Y - 1                           ||
        d->rooms[i].position[dim_x] + d->rooms[i].size[dim_x] > DUNGEON_X - 1 ||
        d->rooms[i].position[dim_x] + d->rooms[i].size[dim_x] < 0             ||
        d->rooms[i].position[dim_y] + d->rooms[i].size[dim_y] > DUNGEON_Y - 1 ||
        d->rooms[i].position[dim_y] + d->rooms[i].size[dim_y] < 0)             {
      fprintf(stderr, "Invalid room position in restored dungeon.\n");

      exit(-1);
    }
        

    /* After reading each room, we need to reconstruct them in the dungeon. */
    for (y = (uint32_t)d->rooms[i].position[dim_y];
         y < (uint32_t)d->rooms[i].position[dim_y] + d->rooms[i].size[dim_y];
         y++) {
      for (x = (uint32_t)d->rooms[i].position[dim_x];
           x < (uint32_t)d->rooms[i].position[dim_x] + d->rooms[i].size[dim_x];
           x++) {
            d->map[y][x] = ter_floor_room;
      }
    }
  }

  return 0;
}

// Function to trim leading and trailing spaces
std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  size_t last = str.find_last_not_of(" \t\n\r");

  if (first == std::string::npos || last == std::string::npos)
      return "";  // If the string is empty or consists only of whitespace.

  return str.substr(first, (last - first + 1));
}

void print_item_descriptions(std::vector<itemDescription_t> *possible_items){
  for (size_t i = 0; i < possible_items->size(); i++) {
    const itemDescription_t& item = (*possible_items)[i];

    std::cout << item.NAME << std::endl;
    std::cout << item.DESC << std::endl;
    std::cout << item.TYPE << std::endl;
    std::cout << item.COLOR << std::endl;




    for(char c : item.WEIGHT){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.HIT){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.ATTR){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.VAL){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.DAM){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.DODGE){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.DEF){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : item.SPEED){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    std::cout << item.ART << std::endl;




    std::cout << item.RRTY << std::endl;



    std::cout << std::endl;


  }

}

void handle_monster_keyword(std::string keyword, std::vector<monsterDescription_t> *possible_monsters, monsterDescription_t *monster, std::string rest){

  rest = trim(rest);
  if (keyword == "NAME") {
    //std::cout << "Handle NAME field\n";

    monster->setNAME(rest);
    // std::cout << "Inside handle_monster_keyword" << std::endl;
    // std::cout << rest << std::endl;

    if(monster->validMonster() == 1){
      possible_monsters->push_back(*monster);
    }


  }

  else if (keyword == "SYMB") {
      //std::cout << "Handle SYMB field\n";
      char tmp = rest[0];

      //std::cout << tmp << std::endl;
      //std::cout << rest << std::endl;


      monster->setSYMB(tmp);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }

  }
  else if (keyword == "COLOR") {
      //std::cout << "Handle COLOR field\n";

      monster->setCOLOR(rest);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else if (keyword == "SPEED") {
      //std::cout << "Handle SPEED field\n";

      std::vector<char> spd;

      for(char c : rest){
        spd.push_back(c);
        //std::cout << c << std::endl;
      }

      monster->setSPEED(spd);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else if (keyword == "DAM") {
      //std::cout << "Handle DAM field\n";

      std::vector<char> dam;

      for(char c : rest){
        dam.push_back(c);
        //std::cout << c << std::endl;
      }

      monster->setDAM(dam);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else if (keyword == "HP") {
      //std::cout << "Handle HP field\n";

      std::vector<char> hp;

      for(char c : rest){
        hp.push_back(c);
        //std::cout << c << std::endl;
      }

      monster->setHP(hp);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else if (keyword == "ABIL") {
      //std::cout << "Handle ABIL field\n";

      std::vector<std::string> abilities;

      std::istringstream stream(rest); 
      std::string ability; 

      
      while (stream >> ability) {
          abilities.push_back(ability);
      }

      monster->setABIL(abilities);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else if (keyword == "RRTY") {
      //std::cout << "Handle RRTY field\n";

      float value = std::stof(rest);

      monster->setRRTY(value);

      if(monster->validMonster() == 1){
        possible_monsters->push_back(*monster);
      }
  }
  else {
      std::cout << "Unknown field: " << keyword << std::endl;
  }

}


void print_monster_descriptions(std::vector<monsterDescription_t> *possible_monsters){
  for (size_t i = 0; i < possible_monsters->size(); i++) {
    const monsterDescription_t& monster = (*possible_monsters)[i];
    std::cout << monster.NAME << std::endl;
    std::cout << monster.DESC << std::endl;
    std::cout << monster.SYMB << std::endl;
    std::cout << monster.COLOR << std::endl;

    for(char c : monster.SPEED){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(std::string ab : monster.ABIL){
      std::cout << ab << " ";
    }
    std::cout << std::endl;

    for(char c : monster.HP){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    for(char c : monster.DAM){
      //spd.push_back(c);
      std::cout << c;
    }
    std::cout << std::endl;

    std::cout << monster.RRTY << std::endl;


    std::cout << std::endl;
  }

}

void handle_item_keyword(std::string keyword, std::vector<itemDescription_t> *possible_items, itemDescription_t *item, std::string rest){

  rest = trim(rest);

  if (keyword == "NAME") {
    //std::cout << "Handle NAME field\n";

    item->setNAME(rest);
    // std::cout << "Inside handle_monster_keyword" << std::endl;
    // std::cout << rest << std::endl;

    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }

  }
  else if (keyword == "TYPE") {

    item->setTYPE(rest);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }

  }
  else if (keyword == "COLOR") {
      //std::cout << "Handle COLOR field\n";

      item->setCOLOR(rest);


      if(item->validItem() == 1){
        possible_items->push_back(*item);
      }
  }
  else if (keyword == "SPEED") {
      //std::cout << "Handle SPEED field\n";

      std::vector<char> spd;

      for(char c : rest){
        spd.push_back(c);
        //std::cout << c << std::endl;
      }

      item->setSPEED(spd);


      if(item->validItem() == 1){
        possible_items->push_back(*item);
      }
  }
  else if (keyword == "DAM") {
      //std::cout << "Handle DAM field\n";

      std::vector<char> dam;

      for(char c : rest){
        dam.push_back(c);
        //std::cout << c << std::endl;
      }

      item->setDAM(dam);


      if(item->validItem() == 1){
        possible_items->push_back(*item);
      }
  }
  else if (keyword == "HIT") {
      //std::cout << "Handle HP field\n";

      std::vector<char> hit;

      for(char c : rest){
        hit.push_back(c);
        //std::cout << c << std::endl;
      }

      item->setHIT(hit);


      if(item->validItem() == 1){
        possible_items->push_back(*item);
      }
  }
  else if (keyword == "DODGE") {
    //std::cout << "Handle HP field\n";

    std::vector<char> dodge;

    for(char c : rest){
      dodge.push_back(c);
      //std::cout << c << std::endl;
    }

    item->setDODGE(dodge);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }
  }
  else if (keyword == "DEF") {
    //std::cout << "Handle HP field\n";

    std::vector<char> def;

    for(char c : rest){
      def.push_back(c);
      //std::cout << c << std::endl;
    }

    item->setDEF(def);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }
  }
  else if (keyword == "WEIGHT") {
    //std::cout << "Handle HP field\n";

    std::vector<char> weight;

    for(char c : rest){
      weight.push_back(c);
      //std::cout << c << std::endl;
    }

    item->setWEIGHT(weight);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }
  }
  else if (keyword == "ATTR") {
    //std::cout << "Handle HP field\n";

    std::vector<char> attr;

    for(char c : rest){
      attr.push_back(c);
      //std::cout << c << std::endl;
    }

    item->setATTR(attr);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }
  }
  else if (keyword == "VAL") {
    //std::cout << "Handle HP field\n";

    std::vector<char> val;

    for(char c : rest){
      val.push_back(c);
      //std::cout << c << std::endl;
    }

    item->setVAL(val);


    if(item->validItem() == 1){
      possible_items->push_back(*item);
    }
  }

  else if (keyword == "RRTY") {
      //std::cout << "Handle RRTY field\n";

      float value = std::stof(rest);

      item->setRRTY(value);

      if(item->validItem() == 1){
        possible_items->push_back(*item);
      }
  }

  else if(keyword == "ART"){
    if(rest == "FALSE"){
      item->setART(false);
    }
    else{
      item->setART(true);
    }
  }
  else {
      std::cout << "Unknown field: " << keyword << std::endl;
  }

}


/*Main functions *********************************************************************************************************/
void dungeon_t::init_dungeon(){
    empty_dungeon(this);
    memset(&this->events, 0, sizeof (this->events));
    heap_init(&this->events, compare_events, event_delete);
}

void dungeon_t::delete_dungeon(){
  free(this->rooms);
  heap_delete(&this->events);
  //memset(this->character, 0, sizeof (this->character));
}

int dungeon_t::gen_dungeon(){
    empty_dungeon(this);

    do {
        make_rooms(this);
    } while (place_rooms(this));

    connect_rooms(this);
    place_stairs(this);

    return 0;
}

void dungeon_t::render_dungeon(char **error_message){
  clear();  // Clear the screen

  if (*error_message) {
      mvprintw(0, 0, "%s", *error_message);
  }

  mvprintw(1, 0, " "); 
  mvprintw(2, 0, " "); 

  pair_t p;

  for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
      for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
          char ch = ' ';
          int color_pair = 0;


          switch (mappair(this, p)) {
              case ter_wall:
              case ter_wall_immutable:
                  ch = ' ';
                  break;
              case ter_floor:
              case ter_floor_room:
                  ch = '.';
                  break;
              case ter_floor_hall:
                  ch = '#';
                  break;
              case ter_debug:
                  ch = '*';
                  mvprintw(0, 0, "Debug character at %d, %d", p[dim_y], p[dim_x]);
                  break;
              case ter_stairs_up:
                  ch = '<';
                  break;
              case ter_stairs_down:
                  ch = '>';
                  break;
              default:
                  ch = ' ';
                  break;
          }


          if (game_objects[p[dim_y]][p[dim_x]]) {
              ch = game_objects[p[dim_y]][p[dim_x]]->SYMB;

              std::string color = game_objects[p[dim_y]][p[dim_x]]->COLOR;
              if      (color == "MAGENTA") color_pair = 5;
              else if (color == "RED")     color_pair = 1;
              else if (color == "BLACK")   color_pair = 7; // white text
              else if (color == "YELLOW")  color_pair = 3;
              else if (color == "CYAN")    color_pair = 6;
              else if (color == "WHITE")   color_pair = 7;
              else if (color == "GREEN")   color_pair = 2;
              else if (color == "BLUE")    color_pair = 4;
          }


          if (charpair(this, p)) {
              ch = character[p[dim_y]][p[dim_x]]->symbol;
              if(character[p[dim_y]][p[dim_x]]->npc){
                std::string color = character[p[dim_y]][p[dim_x]]->npc->COLOR;

                std::stringstream colorStream(color);
                std::string firstColor;
                colorStream >> firstColor;          if (charpair(this, p)) {
                  ch = character[p[dim_y]][p[dim_x]]->symbol;
                  if(character[p[dim_y]][p[dim_x]]->npc){
                    std::string color = character[p[dim_y]][p[dim_x]]->npc->COLOR;
    
                    std::stringstream colorStream(color);
                    std::string firstColor;
                    colorStream >> firstColor;
    
    
                    if (color == "MAGENTA") color_pair = 5;
                    else if (color == "RED")     color_pair = 1;
                    else if (color == "BLACK")   color_pair = 7; // white text
                    else if (color == "YELLOW")  color_pair = 3;
                    else if (color == "CYAN")    color_pair = 6;
                    else if (color == "WHITE")   color_pair = 7;
                    else if (color == "GREEN")   color_pair = 2;
                    else if (color == "BLUE")    color_pair = 4;
                  }
              }
            }
          }


          if (color_pair > 0) attron(COLOR_PAIR(color_pair));
          mvaddch(p[dim_y] + 3, p[dim_x], ch);
          if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
        
      }
  }

  refresh(); // Display updates
}


void dungeon_t::update_player_view_dungeon(){
  //using seenByPlayer[][] and playerxy(), and playerPair()

  //get pc position
  int pcX = this->pc->position[dim_x];
  int pcY = this->pc->position[dim_y];

  //Add current pc position to the seen map
  playerxy(this, pcX, pcY) = mapxy(this, pcX, pcY);

  //Check surrounding 5x5 radius terrain and add it to the map
  playerxy(this, pcX - 2, pcY - 2) = mapxy(this, pcX - 2, pcY - 2);
  playerxy(this, pcX - 1, pcY - 2) = mapxy(this, pcX - 1, pcY - 2);
  playerxy(this, pcX, pcY - 2) = mapxy(this, pcX, pcY - 2);
  playerxy(this, pcX + 1, pcY - 2) = mapxy(this, pcX + 1, pcY - 2);
  playerxy(this, pcX + 2, pcY - 2) = mapxy(this, pcX + 2, pcY - 2);

  playerxy(this, pcX - 2, pcY - 1) = mapxy(this, pcX - 2, pcY - 1);
  playerxy(this, pcX - 1, pcY - 1) = mapxy(this, pcX - 1, pcY - 1);
  playerxy(this, pcX, pcY - 1) = mapxy(this, pcX, pcY - 1);
  playerxy(this, pcX + 1, pcY - 1) = mapxy(this, pcX + 1, pcY - 1);
  playerxy(this, pcX + 2, pcY - 1) = mapxy(this, pcX + 2, pcY - 1);

  playerxy(this, pcX - 2, pcY) = mapxy(this, pcX - 2, pcY);
  playerxy(this, pcX - 1, pcY) = mapxy(this, pcX - 1, pcY);
  playerxy(this, pcX + 1, pcY) = mapxy(this, pcX + 1, pcY);
  playerxy(this, pcX + 2, pcY) = mapxy(this, pcX + 2, pcY);

  playerxy(this, pcX - 2, pcY + 1) = mapxy(this, pcX - 2, pcY + 1);
  playerxy(this, pcX - 1, pcY + 1) = mapxy(this, pcX - 1, pcY + 1);
  playerxy(this, pcX, pcY + 1) = mapxy(this, pcX, pcY + 1);
  playerxy(this, pcX + 1, pcY + 1) = mapxy(this, pcX + 1, pcY + 1);
  playerxy(this, pcX + 2, pcY + 1) = mapxy(this, pcX + 2, pcY + 1);

  playerxy(this, pcX - 2, pcY + 2) = mapxy(this, pcX - 2, pcY + 2);
  playerxy(this, pcX - 1, pcY + 2) = mapxy(this, pcX - 1, pcY + 2);
  playerxy(this, pcX, pcY + 2) = mapxy(this, pcX, pcY + 2);
  playerxy(this, pcX + 1, pcY + 2) = mapxy(this, pcX + 1, pcY + 2);
  playerxy(this, pcX + 2, pcY + 2) = mapxy(this, pcX + 2, pcY + 2);

}

void dungeon_t::render_player_version_dungeon(char **error_message, char **notifications){
  clear();  // Clear the screen before rendering


    // Initialize color if not already done
    if (!has_colors()) {
      mvprintw(0, 0, "Your terminal does not support colors.");
      refresh();
      return;
    }
    //start_color();
    init_pair(8, COLOR_RED, COLOR_BLACK);  // Red text on black background

  // Line 1: Print the error message if provided
  if (*error_message) {
      mvprintw(0, 0, "%s", *error_message);
  }

  //get pc position
  int pcX = this->pc->position[dim_x];
  int pcY = this->pc->position[dim_y];

  //character_t* character[DUNGEON_Y][DUNGEON_X] = {nullptr};

  // Lines 2 & 3: Leave blank
  mvprintw(1, 0, " "); 
  mvprintw(2, 0, " "); 

  pair_t p;

  for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
      for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
          //char ch;
          bool is_border = (p[dim_x] == pcX - 2 || p[dim_x] == pcX + 2 ||
            p[dim_y] == pcY - 2 || p[dim_y] == pcY + 2) &&
           (p[dim_x] >= pcX - 2 && p[dim_x] <= pcX + 2) &&
           (p[dim_y] >= pcY - 2 && p[dim_y] <= pcY + 2);

           char ch;

           //Terrain
           switch (playerPair(this, p)) {
               case ter_not_seen_by_player:
                   ch = ' ';
                   break;
               case ter_wall:
               case ter_wall_immutable:
                   ch = is_border ? '\'' : ' ';
                   break;
               case ter_floor:
               case ter_floor_room:
                   ch = '.';
                   break;
               case ter_floor_hall:
                   ch = '#';
                   break;
               case ter_debug:
                   ch = '*';
                   mvprintw(0, 0, "Debug character at %d, %d", p[dim_y], p[dim_x]);
                   break;
               case ter_stairs_up:
                   ch = '<';
                   break;
               case ter_stairs_down:
                   ch = '>';
                   break;
               default:
                   ch = ' ';
                   break;
           }
           
           //Object
           int color_pair = 0;
           if (game_objects[p[dim_y]][p[dim_x]] &&
               p[dim_y] >= pcY - 2 && p[dim_y] <= pcY + 2 &&
               p[dim_x] >= pcX - 2 && p[dim_x] <= pcX + 2) {
           
               ch = game_objects[p[dim_y]][p[dim_x]]->SYMB;
           
               std::string obj_color = game_objects[p[dim_y]][p[dim_x]]->COLOR;
               if (obj_color == "MAGENTA") color_pair = 5;
               else if (obj_color == "RED") color_pair = 1;
               else if (obj_color == "BLACK" || obj_color == "WHITE") color_pair = 7;
               else if (obj_color == "YELLOW") color_pair = 3;
               else if (obj_color == "CYAN") color_pair = 6;
               else if (obj_color == "GREEN") color_pair = 2;
               else if (obj_color == "BLUE") color_pair = 4;
           }
           
           //Monster
           if (charpair(this, p)  &&
              p[dim_y] >= pcY - 2 && p[dim_y] <= pcY + 2 &&
              p[dim_x] >= pcX - 2 && p[dim_x] <= pcX + 2) {
            ch = character[p[dim_y]][p[dim_x]]->symbol;
            if(character[p[dim_y]][p[dim_x]]->npc){
              std::string color = character[p[dim_y]][p[dim_x]]->npc->COLOR;

              std::stringstream colorStream(color);
              std::string firstColor;
              colorStream >> firstColor;


              if (color == "MAGENTA") color_pair = 5;
              else if (color == "RED")     color_pair = 1;
              else if (color == "BLACK")   color_pair = 7; // white text
              else if (color == "YELLOW")  color_pair = 3;
              else if (color == "CYAN")    color_pair = 6;
              else if (color == "WHITE")   color_pair = 7;
              else if (color == "GREEN")   color_pair = 2;
              else if (color == "BLUE")    color_pair = 4;
            }
        }
           
           // Colors and stuff
           if (color_pair > 0) attron(COLOR_PAIR(color_pair));
           if (is_border && color_pair == 0) attron(COLOR_PAIR(8));
           
           mvaddch(p[dim_y] + 3, p[dim_x], ch);
           
           if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
           if (is_border && color_pair == 0) attroff(COLOR_PAIR(8));

           if (*notifications) {
            mvprintw(DUNGEON_Y + 3, 0, "%s", *notifications);
        }
           
      }
  }

  refresh(); // Refresh the screen to show updates
  notifications = nullptr;
}

int dungeon_t::write_dungeon(char *file){
    const char *home;
    char *filename;
    FILE *f;
    size_t len;
    uint32_t be32;
  
    if (!file) {
      if (!(home = getenv("HOME"))) {
        fprintf(stderr, "\"HOME\" is undefined.  Using working directory.\n");
        home = ".";
      }
  
      len = (strlen(home) + strlen(SAVE_DIR) + strlen(DUNGEON_SAVE_FILE) +
             1 /* The NULL terminator */                                 +
             2 /* The slashes */);
  
      filename = (char*)malloc(len * sizeof (*filename));
      sprintf(filename, "%s/%s/", home, SAVE_DIR);
      makedirectory(filename);
      strcat(filename, DUNGEON_SAVE_FILE);
  
      if (!(f = fopen(filename, "w"))) {
        perror(filename);
        free(filename);
  
        return 1;
      }
      free(filename);
    } else {
      if (!(f = fopen(file, "w"))) {
        perror(file);
        exit(-1);
      }
    }
  
    /* The semantic, which is 6 bytes, 0-11 */
    fwrite(DUNGEON_SAVE_SEMANTIC, 1, sizeof (DUNGEON_SAVE_SEMANTIC) - 1, f);
  
    /* The version, 4 bytes, 12-15 */
    be32 = htobe32(DUNGEON_SAVE_VERSION);
    fwrite(&be32, sizeof (be32), 1, f);
  
    /* The size of the file, 4 bytes, 16-19 */
    be32 = htobe32(calculate_dungeon_size(this));
    fwrite(&be32, sizeof (be32), 1, f);
  
    /* The PC position, 2 bytes, 20-21 */
    fwrite(&this->pc->position[dim_x], 1, 1, f);
    fwrite(&this->pc->position[dim_y], 1, 1, f);
  
    /* The dungeon map, 1680 bytes, 22-1702 */
    write_dungeon_map(this, f);
  
    /* The rooms, num_rooms * 4 bytes, 1703-end */
    write_rooms(this, f);
  
    /* And the stairs */
    write_stairs(this, f);
  
    fclose(f);
  
    return 0;  
}



void dungeon_t::read_monster_descriptions(std::string fileName) {
  std::vector<std::string> words = {"NAME", "SYMB", "COLOR", "DESC", "SPEED", "DAM", "HP", "ABIL", "RRTY"};
  std::string newMonster = "BEGIN MONSTER";
  //const char* endMonster = "END";
  std::string keyword;
  std::string rest_of_line;


  //newMonster = trim(newMonster);

  const char* homeDir = getenv("HOME");

  // int endOfFile = 1;

  if (!homeDir) {
      std::cerr << "\"HOME\" is undefined. Using working directory.\n";
      homeDir = "."; // Default to current directory if HOME is undefined
  }

  std::string dirPath = std::string(homeDir) + "/.rlg327"; 
  std::string filePath = dirPath + "/" + fileName; 

  //std::cout << filePath << std::endl;

  // Check if the directory exists
  if (!std::filesystem::exists(dirPath)) {
      std::cout << "Directory doesn't exist. Creating: " << dirPath << "\n";
      try {
          if (!std::filesystem::create_directory(dirPath)) {
              std::cerr << "Failed to create directory: " << dirPath << "\n";
              return;
          }
      } catch (const std::filesystem::filesystem_error& e) {
          std::cerr << "Error creating directory: " << e.what() << "\n";
          return;
      }
  }

  // Check if the file exists
  struct stat buf;
  if (stat(filePath.c_str(), &buf)) {
      std::cerr << "Error with stat(): " << strerror(errno) << "\n";
      return;
  }

  // Open file with ifstream
  std::ifstream file(filePath);
  if (!file.is_open()) {
      std::cerr << "Could not open file: " << filePath << "\n";
      return;
  }

  // // Read and print the file content
  // std::string line;
  // while (std::getline(file, line)) {
  //     std::cout << line << "\n";
  // }

  std::string headerExpected = "RLG327 MONSTER DESCRIPTION 1";
  std::string headerFile;
  std::getline(file, headerFile);


  headerFile = trim(headerFile);
  headerExpected = trim(headerExpected);

  std::string tmp;


  if(headerExpected == headerFile){
    //std::cout << headerFile << std::endl;
    //while not at end of file
    while(!file.eof()){
      //If newMonster
      std::getline(file, tmp);
      tmp = trim(tmp);

      while (tmp != newMonster && !file.eof()){
        std::getline(file, tmp);
        tmp = trim(tmp);
      }

      if(file.eof()){
        //std::cout << "Keyword: NEW MONSTER was not found" << std::endl;
        break;
      }

      // std::cout << tmp << std::endl;
      //Declare new monster
      monsterDescription_t monster;

      std::getline(file, tmp);
    
      while(tmp.empty()){
        std::getline(file, tmp);
      }


      std::istringstream stream(tmp);
      stream >> keyword;

      keyword = trim(keyword);

      while (keyword != "END"){

        //std::cout << keyword << rest_of_line << std::endl;

        //std::cout << keyword << std::endl;

        // Read the rest of the line
        std::getline(stream, rest_of_line);


        if (!rest_of_line.empty() && rest_of_line[0] == ' ') {
            rest_of_line = rest_of_line.substr(1); 
        }


        //std::cout << "Keyword: " << keyword << std::endl;
        //std::cout << "Rest of the line: " << rest_of_line << std::endl;

        if (std::find(words.begin(), words.end(), keyword) != words.end()) {
            //std::cout << "Match Found" << std::endl;
            //std::cout << tmp << std::endl;

            if (keyword == "DESC") {
              std::string description;
              std::string line;
              bool valid = true;
            
              std::getline(file, line);
              line = trim(line);
            
              while (line != ".") {
                if (line.length() > 77) {
                  std::cerr << "Error: Description line exceeds 77 characters.\n";
                  valid = false;
                }
                description += line + "\n";
                std::getline(file, line);
                line = trim(line);
              }
            
              if (valid) {
                monster.setDESC(trim(description));
              } else {
                monster.setDESC("INVALID DESCRIPTION");
              }
            } else {
              handle_monster_keyword(keyword, this->possible_monsters, &monster, rest_of_line);
            }
        }

        //std::cout << "Outside handle_monster_keyword" << std::endl;
        //std::cout << monster.NAME << std::endl;


        std::getline(file, tmp);
    
        while(tmp.empty()){
          std::getline(file, tmp);
        }

        //std::cout << tmp << std::endl;

        std::istringstream stream(tmp);
        stream >> keyword;
  
        keyword = trim(keyword);

        std::getline(stream, rest_of_line);


    }

        //Check for stuff until END

        // endOfFile = 0;
    }

    //print_monster_descriptions(this->possible_monsters);

  }else{
    std::cout << "Headers do not match :: Exiting Program" << std::endl;
  }

  // std::cout << headerExpected << std::endl;
  // std::cout << headerFile << std::endl;




  file.close();
}

void dungeon_t::read_item_descriptions(std::string fileName) {
  std::vector<std::string> words = {"NAME", "DESC", "TYPE", "COLOR", "HIT", "DAM", "DODGE", "DEF", "WEIGHT", "SPEED", "ATTR", "VAL", "ART", "RRTY"};
  std::string newObject = "BEGIN OBJECT";
  //const char* endMonster = "END";
  std::string keyword;
  std::string rest_of_line;


  //newMonster = trim(newMonster);

  const char* homeDir = getenv("HOME");

  // int endOfFile = 1;

  if (!homeDir) {
      std::cerr << "\"HOME\" is undefined. Using working directory.\n";
      homeDir = "."; // Default to current directory if HOME is undefined
  }

  std::string dirPath = std::string(homeDir) + "/.rlg327"; 
  std::string filePath = dirPath + "/" + fileName; 

  //std::cout << filePath << std::endl;

  // Check if the directory exists
  if (!std::filesystem::exists(dirPath)) {
      std::cout << "Directory doesn't exist. Creating: " << dirPath << "\n";
      try {
          if (!std::filesystem::create_directory(dirPath)) {
              std::cerr << "Failed to create directory: " << dirPath << "\n";
              return;
          }
      } catch (const std::filesystem::filesystem_error& e) {
          std::cerr << "Error creating directory: " << e.what() << "\n";
          return;
      }
  }

  // Check if the file exists
  struct stat buf;
  if (stat(filePath.c_str(), &buf)) {
      std::cerr << "Error with stat(): " << strerror(errno) << "\n";
      return;
  }

  // Open file with ifstream
  std::ifstream file(filePath);
  if (!file.is_open()) {
      std::cerr << "Could not open file: " << filePath << "\n";
      return;
  }

  // // Read and print the file content
  // std::string line;
  // while (std::getline(file, line)) {
  //     std::cout << line << "\n";
  // }

  std::string headerExpected = "RLG327 OBJECT DESCRIPTION 1";
  std::string headerFile;
  std::getline(file, headerFile);


  headerFile = trim(headerFile);
  headerExpected = trim(headerExpected);

  std::string tmp;


  if(headerExpected == headerFile){
    //std::cout << headerFile << std::endl;
    //while not at end of file
    while(!file.eof()){
      //If newMonster
      std::getline(file, tmp);
      tmp = trim(tmp);

      while (tmp != newObject && !file.eof()){
        std::getline(file, tmp);
        tmp = trim(tmp);
      }

      if(file.eof()){
        //std::cout << "Keyword: NEW MONSTER was not found" << std::endl;
        break;
      }

      // std::cout << tmp << std::endl;
      //Declare new item
      itemDescription_t item;

      std::getline(file, tmp);
    
      while(tmp.empty()){
        std::getline(file, tmp);
      }


      std::istringstream stream(tmp);
      stream >> keyword;

      keyword = trim(keyword);

      while (keyword != "END"){

        //std::cout << keyword << rest_of_line << std::endl;

        //std::cout << keyword << std::endl;

        // Read the rest of the line
        std::getline(stream, rest_of_line);


        if (!rest_of_line.empty() && rest_of_line[0] == ' ') {
            rest_of_line = rest_of_line.substr(1); 
        }


        //std::cout << "Keyword: " << keyword << std::endl;
        //std::cout << "Rest of the line: " << rest_of_line << std::endl;

        if (std::find(words.begin(), words.end(), keyword) != words.end()) {
            //std::cout << "Match Found" << std::endl;
            //std::cout << tmp << std::endl;

            if (keyword == "DESC") {
              std::string description;
              std::string line;
              bool valid = true;
            
              std::getline(file, line);
              line = trim(line);
            
              while (line != ".") {
                if (line.length() > 77) {
                  std::cerr << "Error: Description line exceeds 77 characters.\n";
                  valid = false;
                }
                description += line + "\n";
                std::getline(file, line);
                line = trim(line);
              }
            
              if (valid) {
                item.setDESC(trim(description));
              }

            } else {
              handle_item_keyword(keyword, this->possible_items, &item, rest_of_line);
            }
        }

        //std::cout << "Outside handle_monster_keyword" << std::endl;
        //std::cout << monster.NAME << std::endl;


        std::getline(file, tmp);
    
        while(tmp.empty()){
          std::getline(file, tmp);
        }

        //std::cout << tmp << std::endl;

        std::istringstream stream(tmp);
        stream >> keyword;
  
        keyword = trim(keyword);

        std::getline(stream, rest_of_line);


    }

        //Check for stuff until END

        // endOfFile = 0;
    }

    //print_item_descriptions(this->possible_items);

  }else{
    std::cout << "Headers do not match :: Exiting Program" << std::endl;
  }

  // std::cout << headerExpected << std::endl;
  // std::cout << headerFile << std::endl;




  file.close();
}

int dungeon_t::read_dungeon(char *file){
    char semantic[sizeof (DUNGEON_SAVE_SEMANTIC)];
    uint32_t be32;
    FILE *f;
    const char *home;
    size_t len;
    char *filename;
    struct stat buf;
  
    if (!file) {
      if (!(home = getenv("HOME"))) {
        fprintf(stderr, "\"HOME\" is undefined.  Using working directory.\n");
        home = ".";
      }
  
      len = (strlen(home) + strlen(SAVE_DIR) + strlen(DUNGEON_SAVE_FILE) +
             1 /* The NULL terminator */                                 +
             2 /* The slashes */);
  
      filename = (char*)malloc(len * sizeof (*filename));
      sprintf(filename, "%s/%s/%s", home, SAVE_DIR, DUNGEON_SAVE_FILE);
  
      if (!(f = fopen(filename, "r"))) {
        perror(filename);
        free(filename);
        exit(-1);
      }
  
      if (stat(filename, &buf)) {
        perror(filename);
        exit(-1);
      }
  
      free(filename);
    } else {
      if (!(f = fopen(file, "r"))) {
        perror(file);
        exit(-1);
      }
      if (stat(file, &buf)) {
        perror(file);
        exit(-1);
      }
    }
  
    this->num_rooms = 0;
  
    fread(semantic, sizeof (DUNGEON_SAVE_SEMANTIC) - 1, 1, f);
    semantic[sizeof (DUNGEON_SAVE_SEMANTIC) - 1] = '\0';
    if (strncmp(semantic, DUNGEON_SAVE_SEMANTIC,
            sizeof (DUNGEON_SAVE_SEMANTIC) - 1)) {
      fprintf(stderr, "Not an RLG327 save file.\n");
      exit(-1);
    }
    fread(&be32, sizeof (be32), 1, f);
    if (be32toh(be32) != 0) { /* Since we expect zero, be32toh() is a no-op. */
      fprintf(stderr, "File version mismatch.\n");
      exit(-1);
    }
    fread(&be32, sizeof (be32), 1, f);
    if (buf.st_size != be32toh(be32)) {
      fprintf(stderr, "File size mismatch.\n");
      exit(-1);
    }
  
    fread(&this->pc->position[dim_x], 1, 1, f);
    fread(&this->pc->position[dim_y], 1, 1, f);
    
    read_dungeon_map(this, f);
  
    read_rooms(this, f);
  
    read_stairs(this, f);
  
    fclose(f);
  
    return 0;
}

int dungeon_t::read_pgm(char *pgm){
    FILE *f;
    char s[80];
    uint8_t gm[DUNGEON_Y - 2][DUNGEON_X - 2];
    uint32_t x, y;
    uint32_t i;
    char size[8]; /* Big enough to hold two 3-digit values with a space between. */
  
    if (!(f = fopen(pgm, "r"))) {
      perror(pgm);
      exit(-1);
    }
  
    if (!fgets(s, 80, f) || strncmp(s, "P5", 2)) {
      fprintf(stderr, "Expected P5\n");
      exit(-1);
    }
    if (!fgets(s, 80, f) || s[0] != '#') {
      fprintf(stderr, "Expected comment\n");
      exit(-1);
    }
    snprintf(size, 8, "%d %d", DUNGEON_X - 2, DUNGEON_Y - 2);
    if (!fgets(s, 80, f) || strncmp(s, size, 5)) {
      fprintf(stderr, "Expected %s\n", size);
      exit(-1);
    }
    if (!fgets(s, 80, f) || strncmp(s, "255", 2)) {
      fprintf(stderr, "Expected 255\n");
      exit(-1);
    }
  
    fread(gm, 1, (DUNGEON_X - 2) * (DUNGEON_Y - 2), f);
  
    fclose(f);
  
    /* In our gray map, treat black (0) as corridor, white (255) as room, *
     * all other values as a hardness.  For simplicity, treat every white *
     * cell as its own room, so we have to count white after reading the  *
     * image in order to allocate the room array.                         */
    for (this->num_rooms = 0, y = 0; y < DUNGEON_Y - 2; y++) {
      for (x = 0; x < DUNGEON_X - 2; x++) {
        if (!gm[y][x]) {
          this->num_rooms++;
        }
      }
    }
    this->rooms = (room_t*)malloc(sizeof (*this->rooms) * this->num_rooms);
  
    for (i = 0, y = 0; y < DUNGEON_Y - 2; y++) {
      for (x = 0; x < DUNGEON_X - 2; x++) {
        if (!gm[y][x]) {
          this->rooms[i].position[dim_x] = x + 1;
          this->rooms[i].position[dim_y] = y + 1;
          this->rooms[i].size[dim_x] = 1;
          this->rooms[i].size[dim_y] = 1;
          i++;
          this->map[y + 1][x + 1] = ter_floor_room;
          this->hardness[y + 1][x + 1] = 0;
        } else if (gm[y][x] == 255) {
          this->map[y + 1][x + 1] = ter_floor_hall;
          this->hardness[y + 1][x + 1] = 0;
        } else {
          this->map[y + 1][x + 1] = ter_wall;
          this->hardness[y + 1][x + 1] = gm[y][x];
        }
      }
    }
  
    for (x = 0; x < DUNGEON_X; x++) {
      this->map[0][x] = ter_wall_immutable;
      this->hardness[0][x] = 255;
      this->map[DUNGEON_Y - 1][x] = ter_wall_immutable;
      this->hardness[DUNGEON_Y - 1][x] = 255;
    }
    for (y = 1; y < DUNGEON_Y - 1; y++) {
      this->map[y][0] = ter_wall_immutable;
      this->hardness[y][0] = 255;
      this->map[y][DUNGEON_X - 1] = ter_wall_immutable;
      this->hardness[y][DUNGEON_X - 1] = 255;
    }
  
    return 0;
}

void dungeon_t::render_hardness_map(){
  /* The hardness map includes coordinates, since it's larger *
   * size makes it more difficult to index a position by eye. */
  
   pair_t p;
   int i;
   
   putchar('\n');
   printf("   ");
   for (i = 0; i < DUNGEON_X; i++) {
     printf("%2d", i);
   }
   putchar('\n');
   for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
     printf("%2d ", p[dim_y]);
     for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
       printf("%02x", hardnesspairAnotherVersion(this, p));
     }
     putchar('\n');
   }    
}

void dungeon_t::render_tunnel_distance_map(){
    pair_t p;

    for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
      for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
        if (p[dim_x] ==  this->pc->position[dim_x] &&
            p[dim_y] ==  this->pc->position[dim_y]) {
          putchar('@');
        } else {
          switch (mappair(this, p)) {
          case ter_wall_immutable:
            putchar(' ');
            break;
          case ter_wall:
          case ter_floor:
          case ter_floor_room:
          case ter_floor_hall:
          case ter_stairs:
          case ter_stairs_up:
          case ter_stairs_down:
            /* Placing X for infinity */
            if (this->pc_tunnel[p[dim_y]][p[dim_x]] == UCHAR_MAX) {
              putchar('X');
            } else {
              putchar('0' + this->pc_tunnel[p[dim_y]][p[dim_x]] % 10);
            }
            break;
          case ter_debug:
            fprintf(stderr, "Debug character at %d, %d\n", p[dim_y], p[dim_x]);
            putchar('*');
            break;
          case ter_not_seen_by_player:
            //Will not be adding this terrain type to map so this should not be ever entered
            break;
          }
        }
      }
      putchar('\n');
    }    
}

void dungeon_t::render_distance_map(){
    pair_t p;

    for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
      for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
        if (p[dim_x] ==  this->pc->position[dim_x] &&
            p[dim_y] ==  this->pc->position[dim_y]) {
          putchar('@');
        } else {
          switch (mappair(this, p)) {
          case ter_wall:
          case ter_wall_immutable:
            putchar(' ');
            break;
          case ter_floor:
          case ter_floor_room:
          case ter_floor_hall:
          case ter_stairs:
          case ter_stairs_up:
          case ter_stairs_down:
            /* Placing X for infinity */
            if (this->pc_distance[p[dim_y]][p[dim_x]] == UCHAR_MAX) {
              putchar('X');
            } else {
              putchar('0' + this->pc_distance[p[dim_y]][p[dim_x]] % 10);
            }
            break;
          case ter_debug:
            fprintf(stderr, "Debug character at %d, %d\n", p[dim_y], p[dim_x]);
            putchar('*');
            break;
          case ter_not_seen_by_player:
            //Will not be adding this terrain type to map so this should not be ever entered
            break;
          }
        }
      }
      putchar('\n');
    }    
}

void dungeon_t::render_movement_cost_map(){
    pair_t p;

    putchar('\n');
    for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
      for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
        if (p[dim_x] ==  this->pc->position[dim_x] &&
            p[dim_y] ==  this->pc->position[dim_y]) {
          putchar('@');
        } else {
          if (hardnesspairAnotherVersion(this, p) == 255) {
            printf("X");
          } else {
            printf("%d", (hardnesspairAnotherVersion(this, p) / 85) + 1);
          }
        }
      }
      putchar('\n');
    }
}



void dungeon_t::display_message_popup(const std::vector<std::string>& message, const std::string& title) {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 70;
  int content_width = box_width - 4;
  int start_y, start_x;
  int box_height = 20;  // Initial guess, will recalc based on wrapped lines

  // Helper to wrap a single string
  auto wrap_text = [](const std::string& text, int max_width) -> std::vector<std::string> {
    std::istringstream iss(text);
    std::string word, line;
    std::vector<std::string> wrapped;

    while (iss >> word) {
      if (line.length() + word.length() + 1 > (size_t)max_width) {
        wrapped.push_back(line);
        line = word;
      } else {
        if (!line.empty()) line += " ";
        line += word;
      }
    }
    if (!line.empty()) wrapped.push_back(line);
    return wrapped;
  };

  // Wrap all lines in the message
  std::vector<std::string> wrapped_lines;
  for (const std::string& line : message) {
    std::vector<std::string> w = wrap_text(line, content_width);
    wrapped_lines.insert(wrapped_lines.end(), w.begin(), w.end());
  }

  // Now that we know how many lines we need
  box_height = std::min((int)wrapped_lines.size() + 6, 20);
  start_y = (term_rows - box_height) / 2;
  start_x = (term_cols - box_width) / 2;

  WINDOW *win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  mvwprintw(win, 1, 2, "%s (Press any key to close)", title.c_str());

  for (int i = 0; i < (int)wrapped_lines.size() && i < box_height - 4; i++) {
    mvwprintw(win, i + 3, 2, "%s", wrapped_lines[i].c_str());
  }

  wrefresh(win);
  wgetch(win);  // Wait for any key
  delwin(win);
  clear();
}




// Generic selection window for carry or equipment slots
int dungeon_t::display_selection_prompt(const std::vector<std::string>& options, const std::string& title) {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 60;
  int box_height = std::min((int)options.size() + 6, 20);
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW *win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  int highlight = 0;
  int ch;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "%s (ESC to cancel, ENTER to select)", title.c_str());

      for (int i = 0; i < (int)options.size(); i++) {
          if (i == highlight) {
              wattron(win, A_REVERSE);
          }
          mvwprintw(win, i + 3, 2, "%d - %s", i, options[i].c_str());
          wattroff(win, A_REVERSE);
      }

      wrefresh(win);

      ch = wgetch(win);
      switch (ch) {
          case KEY_UP:
              if (highlight > 0) highlight--;
              break;
          case KEY_DOWN:
              if (highlight < (int)options.size() - 1) highlight++;
              break;
          case 27:  // ESC
              delwin(win);
              clear();
              return -1;
          case '\n':
          case KEY_ENTER:
              delwin(win);
              clear();
              return highlight;
      }

  } while (true);
}

EquipmentSlot string_to_equipment_slot(const std::string& type_str, pc_t* pc, char **notifications) {
  static std::unordered_map<std::string, EquipmentSlot> type_map = {
      {"WEAPON", EquipmentSlot::WEAPON},
      {"OFFHAND", EquipmentSlot::OFFHAND},
      {"RANGED", EquipmentSlot::RANGED},
      {"ARMOR", EquipmentSlot::ARMOR},
      {"HELMET", EquipmentSlot::HELMET},
      {"CLOAK", EquipmentSlot::CLOAK},
      {"GLOVES", EquipmentSlot::GLOVES},
      {"BOOTS", EquipmentSlot::BOOTS},
      {"AMULET", EquipmentSlot::AMULET},
      {"LIGHT", EquipmentSlot::LIGHT},
      {"RING1", EquipmentSlot::RING1},
      {"RING2", EquipmentSlot::RING2}
  };

  if (type_str == "RING") {
      const auto& equipment = pc->get_equipment();
      if (!equipment[static_cast<size_t>(EquipmentSlot::RING1)]) {
          return EquipmentSlot::RING1;
      } else if (!equipment[static_cast<size_t>(EquipmentSlot::RING2)]) {
          return EquipmentSlot::RING2;
      } else {
          *notifications = const_cast<char*>("No empty ring slot available \n");
          return EquipmentSlot::COUNT;
      }
  }

  auto it = type_map.find(type_str);
  if (it != type_map.end()) {
      return it->second;
  } else {
      // throw std::runtime_error("Invalid equipment slot type: " + type_str);
      *notifications = const_cast<char*>("Invalid Equipment slot type \n");
      return EquipmentSlot::COUNT;
  }
}


void dungeon_t::display_pc_stats() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 40;
  int box_height = 10;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW* win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  int hp = this->pc->HP;
  int speed = this->pc->speed;


  mvwprintw(win, 1, 2, "Player Stats:");
  mvwprintw(win, 3, 4, "HP: %d", hp);
  mvwprintw(win, 4, 4, "Speed: %d", speed);

  mvwprintw(win, 7, 2, "Press any key to continue...");
  wrefresh(win);
  wgetch(win);

  delwin(win);
  clear();
}


void dungeon_t::prompt_wear_item(char **notifications) {
  std::vector<std::string> carry_options;
  for (int i = 0; i < (int)this->pc->carry.size(); i++) {
      Objects* obj = this->pc->carry[i];
      if (obj) {
          carry_options.push_back(obj->NAME + " (" + std::to_string(obj->WEIGHT) + "kg)");
      } else {
          carry_options.push_back("-- empty --");
      }
  }

  int choice = display_selection_prompt(carry_options, "Select Carry Slot to Wear");
  if (choice == -1) return;

  Objects* selected = this->pc->carry[choice];
  if (!selected) return;

  EquipmentSlot slot = string_to_equipment_slot(selected->TYPE, this->pc, notifications);


  // If something is already in the slot, swap
  Objects* old_equipped = this->pc->get_equipment()[static_cast<int>(slot)];
  this->pc->carry[choice] = old_equipped;
  this->pc->set_equipment(slot, selected);
  int newSpeed = selected->getSpeed();
  this->pc->increaseSpeed(newSpeed);
  int hp = selected->getDefense();
  this->pc->increaseHP(hp);
}

void dungeon_t::prompt_take_off_item() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 50;
  int box_height = 20;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW* win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  std::vector<std::pair<int, Objects*>> equipped;
  for (int i = 0; i < static_cast<int>(EquipmentSlot::COUNT); i++) {
      Objects* item = this->pc->get_equipped_item(static_cast<EquipmentSlot>(i));

      if (item) {
          equipped.emplace_back(i, item);
      }
  }

  if (equipped.empty()) {
      mvwprintw(win, 1, 2, "No items are equipped.");
      wrefresh(win);
      wgetch(win);
      delwin(win);
      clear();
      return;
  }

  int selection = 0;
  int ch;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Select equipment to take off (ESC to cancel)");

      for (size_t i = 0; i < equipped.size(); ++i) {
          const auto& [slotIndex, item] = equipped[i];
          if (static_cast<int>(i) == selection) {
              wattron(win, A_REVERSE);
          }
          mvwprintw(win, i + 3, 2, "[%d] %s (%dkg)", slotIndex, item->NAME.c_str(), item->WEIGHT);
          if (static_cast<int>(i) == selection) {
              wattroff(win, A_REVERSE);
          }
      }

      wrefresh(win);
      ch = wgetch(win);

      if (ch == KEY_UP && selection > 0) selection--;
      if (ch == KEY_DOWN && selection < static_cast<int>(equipped.size()) - 1) selection++;

  } while (ch != 27 && ch != '\n');  // ESC or ENTER

  if (ch == '\n') {
      const auto& [slotIndex, item] = equipped[selection];
      if (this->pc->add_to_carry(item)) {
        this->pc->remove_equipped_item(static_cast<EquipmentSlot>(slotIndex));
        int newSpeed = item->getSpeed();
        this->pc->descreaseSpeed(newSpeed);
        int hp = item->getDefense();
        this->pc->decreaseHP(hp);
      } else {
          mvwprintw(win, box_height - 3, 2, "Inventory full!");
          wrefresh(win);
          wgetch(win);
      }
  }

  delwin(win);
  clear();
}

void dungeon_t::prompt_drop_item() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 50;
  int box_height = 20;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW* win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  std::vector<std::pair<int, Objects*>> items;
  for (int i = 0; i < 10; ++i) {
      if (this->pc->carry[i]) {
          items.emplace_back(i, this->pc->carry[i]);
      }
  }

  if (items.empty()) {
      mvwprintw(win, 1, 2, "No items to drop.");
      wrefresh(win);
      wgetch(win);
      delwin(win);
      clear();
      return;
  }

  int selection = 0;
  int ch;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Select item to drop (ESC to cancel)");

      for (size_t i = 0; i < items.size(); ++i) {
          const auto& [index, item] = items[i];
          if (static_cast<int>(i) == selection) {
              wattron(win, A_REVERSE);
          }
          mvwprintw(win, i + 3, 2, "[%d] %s (%dkg)", index, item->NAME.c_str(), item->WEIGHT);
          if (static_cast<int>(i) == selection) {
              wattroff(win, A_REVERSE);
          }
      }

      wrefresh(win);
      ch = wgetch(win);

      if (ch == KEY_UP && selection > 0) selection--;
      if (ch == KEY_DOWN && selection < static_cast<int>(items.size()) - 1) selection++;

  } while (ch != 27 && ch != '\n');  // ESC or ENTER

  if (ch == '\n') {
      const auto& [index, item] = items[selection];
      int y = this->pc->position[dim_y];
      int x = this->pc->position[dim_x];

      if (this->game_objects[y][x]) {
          mvwprintw(win, box_height - 3, 2, "There is already an item on the floor!");
          wrefresh(win);
          wgetch(win);
      } else {
          this->game_objects[y][x] = item;
          this->pc->carry[index] = nullptr;
      }
  }

  delwin(win);
  clear();
}

void dungeon_t::prompt_inspect_item() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 50;
  int box_height = 20;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW* win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  std::vector<std::pair<int, Objects*>> items;
  for (int i = 0; i < 10; ++i) {
      if (this->pc->carry[i]) {
          items.emplace_back(i, this->pc->carry[i]);
      }
  }

  if (items.empty()) {
      mvwprintw(win, 1, 2, "No items.");
      wrefresh(win);
      wgetch(win);
      delwin(win);
      clear();
      return;
  }

  int selection = 0;
  int ch;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Select item to expunge (ESC to cancel)");

      for (size_t i = 0; i < items.size(); ++i) {
          const auto& [index, item] = items[i];
          if (static_cast<int>(i) == selection) {
              wattron(win, A_REVERSE);
          }
          mvwprintw(win, i + 3, 2, "[%d] %s (%dkg)", index, item->NAME.c_str(), item->WEIGHT);
          if (static_cast<int>(i) == selection) {
              wattroff(win, A_REVERSE);
          }
      }

      wrefresh(win);
      ch = wgetch(win);

      if (ch == KEY_UP && selection > 0) selection--;
      if (ch == KEY_DOWN && selection < static_cast<int>(items.size()) - 1) selection++;

  } while (ch != 27 && ch != '\n');  // ESC or ENTER

  if (ch == '\n') {
      const auto& [index, item] = items[selection];

      // Expunge the item: delete it and remove from inventory
      unique_objects->push_back(*item);
      delete item;
      this->pc->carry[index] = nullptr;

      mvwprintw(win, box_height - 3, 2, "Item has been expunged.");
      wrefresh(win);
      wgetch(win);
  }

  delwin(win);
  clear();
}

void wrap_print(WINDOW* win, int start_y, int start_x, int width, const std::string& text) {
  std::istringstream iss(text);
  std::string word;
  size_t x = start_x, y = start_y; // Change x to size_t for comparison

  while (iss >> word) {
      if (x + word.length() >= static_cast<size_t>(start_x + width)) {  // Cast to size_t
          y++;
          x = start_x;
      }
      mvwprintw(win, y, x, "%s", word.c_str());
      x += word.length() + 1;
  }
}


void dungeon_t::prompt_view_item() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 50;
  int box_height = 20;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW* win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  std::vector<std::pair<int, Objects*>> items;
  for (int i = 0; i < 10; ++i) {
      if (this->pc->carry[i]) {
          items.emplace_back(i, this->pc->carry[i]);
      }
  }

  if (items.empty()) {
      mvwprintw(win, 1, 2, "No items to view.");
      wrefresh(win);
      wgetch(win);
      delwin(win);
      clear();
      return;
  }

  int selection = 0;
  int ch;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Select item (ENTER to see more, ESC to exit)");

      // Display items
      int list_height = 10;
      for (size_t i = 0; i < items.size() && i < (size_t)list_height; ++i) {
          const auto& [index, item] = items[i];
          if ((int)i == selection) wattron(win, A_REVERSE);
          mvwprintw(win, i + 3, 2, "[%d] %s", index, item->NAME.c_str());
          if ((int)i == selection) wattroff(win, A_REVERSE);
      }

      const auto& [index, item] = items[selection];

      wrefresh(win);
      ch = wgetch(win);

      if (ch == KEY_UP && selection > 0) selection--;
      if (ch == KEY_DOWN && selection < (int)items.size() - 1) selection++;

      if (ch == '\n') {
          // Open detailed view window
          WINDOW* detail = newwin(box_height, box_width, start_y, start_x);
          box(detail, 0, 0);
          keypad(detail, TRUE);
          mvwprintw(detail, 1, 2, "Extended Info (press any key to go back)");

          mvwprintw(detail, 3, 2, "Hit Bonus: %d",   item->HIT);
          mvwprintw(detail, 4, 2, "Dodge Bonus: %d", item->DODGE);
          mvwprintw(detail, 5, 2, "Defense: %d",     item->DEF);
          mvwprintw(detail, 6, 2, "Speed Bonus: %d", item->SPEED);
          mvwprintw(detail, 8, 2, "Description:");
          wrap_print(detail, 9, 2, box_width - 4, item->DESC);

          wrefresh(detail);
          wgetch(detail);
          delwin(detail);
      }

  } while (ch != 27); // ESC

  delwin(win);
  clear();
}








void dungeon_t::display_pc_equipment() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 60;
  int box_height = 18;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW *win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  const char* equip_labels[] = {
      "Weapon", "Offhand", "Ranged", "Armor",
      "Helmet", "Cloak", "Gloves", "Boots",
      "Amulet", "Light", "Ring 1", "Ring 2"
  };

  auto& equipment = this->pc->get_equipment();

  size_t scroll_offset = 0;
  int ch;
  const size_t max_display = box_height - 4;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Equipped Items (ESC to exit)");

      for (size_t i = 0; i < max_display && i + scroll_offset < equipment.size(); i++) {
          Objects* obj = equipment[i + scroll_offset];
          const char* label = equip_labels[i + scroll_offset];

          if (obj) {
              mvwprintw(win, i + 3, 2, "[%s] %c %s (%dkg)", 
                        label,
                        obj->SYMB,
                        obj->NAME.c_str(),
                        obj->WEIGHT);
          } else {
              mvwprintw(win, i + 3, 2, "[%s] -- empty --", label);
          }
      }

      mvwprintw(win, box_height - 2, 2, "[UP/DOWN to scroll]");
      wrefresh(win);

      ch = wgetch(win);
      if (ch == KEY_DOWN && scroll_offset < equipment.size() - max_display) {
          scroll_offset++;
      } else if (ch == KEY_UP && scroll_offset > 0) {
          scroll_offset--;
      }

  } while (ch != 27);  // ESC

  delwin(win);
  clear();
}


void dungeon_t::display_pc_carry() {
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  int box_width = 60;
  int box_height = 16;
  int start_y = (term_rows - box_height) / 2;
  int start_x = (term_cols - box_width) / 2;

  WINDOW *win = newwin(box_height, box_width, start_y, start_x);
  box(win, 0, 0);
  keypad(win, TRUE);

  auto& carry = this->pc->carry;

  size_t scroll_offset = 0;
  int ch;
  const size_t max_display = box_height - 4;

  do {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, 2, "Carried Items (ESC to exit)");

      for (size_t i = 0; i < max_display && i + scroll_offset < carry.size(); i++) {
          Objects* obj = carry[i + scroll_offset];

          if (obj) {
              mvwprintw(win, i + 3, 2, "[%zu] %c %s (%dkg) [%s]", 
                        i + scroll_offset,
                        obj->SYMB,
                        obj->NAME.c_str(),
                        obj->WEIGHT,
                        obj->TYPE.c_str());
          } else {
              mvwprintw(win, i + 3, 2, "[%zu] -- empty --", i + scroll_offset);
          }
      }

      mvwprintw(win, box_height - 2, 2, "[UP/DOWN to scroll]");
      wrefresh(win);

      ch = wgetch(win);
      if (ch == KEY_DOWN && scroll_offset < carry.size() - max_display) {
          scroll_offset++;
      } else if (ch == KEY_UP && scroll_offset > 0) {
          scroll_offset--;
      }

  } while (ch != 27);  // ESC

  delwin(win);
  clear();
}





void dungeon_t::display_monster_list(){
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);  // Get terminal size

    int box_width = 40;
    int box_height = 15;
    int start_y = (term_rows - box_height) / 2;
    int start_x = (term_cols - box_width) / 2;

    // Create a window for the monster list
    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    box(win, 0, 0);  // Draw border
    keypad(win, TRUE);  // Enable arrow key input

    character_t *monster_list[DUNGEON_Y * DUNGEON_X];
    int num_monsters = 0;

    // Collect all monsters into an array
    for (int y = 0; y < DUNGEON_Y; y++) {
        for (int x = 0; x < DUNGEON_X; x++) {
            if (this->character[y][x] && this->character[y][x] != this->pc) {
                monster_list[num_monsters++] = this->character[y][x];
            }
        }
    }

    int scroll_offset = 0;
    int max_display = box_height - 4;  // Room for border & instructions
    int ch;

    do {
        werase(win);
        box(win, 0, 0);  // Redraw border

        mvwprintw(win, 1, 2, "Monster List (ESC to exit)");

        for (int i = 0; i < max_display && i + scroll_offset < num_monsters; i++) {
            character_t *m = monster_list[i + scroll_offset];
            mvwprintw(win, i + 3, 2, "%c, %dN %dW",
                      m->symbol, 
                      m->position[dim_y] - this->pc->position[dim_y], 
                      m->position[dim_x] - this->pc->position[dim_x]);
        }

        mvwprintw(win, box_height - 2, 2, "[UP/DOWN to scroll]");
        wrefresh(win);

        ch = wgetch(win);
        if (ch == KEY_DOWN && scroll_offset < num_monsters - max_display) {
            scroll_offset++;
        } else if (ch == KEY_UP && scroll_offset > 0) {
            scroll_offset--;
        }

    } while (ch != 27);  // ESC key to exit

    delwin(win);  // Delete the window
    clear();  
    //refresh();  // Restore the dungeon screen
}

int dungeon_t::replace_dungeon(){
    dungeon_t newD;

    newD.gen_dungeon();

    *this = newD;

    return 0;

}

void dungeon_t::clear_dungeon_for_reset(){
    // Clear the map, set all tiles to walls (or your default "empty" state)
    for (int y = 0; y < DUNGEON_Y; y++) {
        for (int x = 0; x < DUNGEON_X; x++) {
            this->map[y][x] = ter_wall;      // Set all tiles to walls (or change to your default "empty" state)
            this->hardness[y][x] = 0;        // Reset hardness (or set to your default value)
            this->character[y][x] = NULL;    // Clear any characters in the dungeon
            this->seenByPlayer[y][x] = ter_not_seen_by_player;
            this->game_objects[y][x] = nullptr;
        }
    }
    // Reset the PC's position if needed (if the player is a character in your game)
    this->pc->position[dim_x] = 0;
    this->pc->position[dim_y] = 0;
    this->pc->alive = 1;  // Reset the player's alive status if needed
  
}

void dungeon_t::gen_objects() {
  if (this->num_rooms <= 1) {
    fprintf(stderr, "Too few rooms to place objects.\n");
    return;
  }

  int randNum = rand() % (MAX_OBJECTS - MIN_OBJECTS + 1) + MIN_OBJECTS;
  int i, j;
  pair_t p;
  uint32_t room;

  for (i = 0; i < randNum; i++) {
    if (this->possible_items->empty()) {
      fprintf(stderr, "No possible items to generate.\n");
      break;
    }

    int randIndex = rand() % this->possible_items->size();
    itemDescription_t potentialItem = (*this->possible_items)[randIndex];

    bool overallValidity = false;
    int attempts = 0;

    while (!overallValidity && attempts++ < 1000) {
      bool validART = true;

      if (potentialItem.ART) {
        validART = true;
        for (j = 0; j < (int)unique_objects->size(); j++) {
          if (potentialItem.NAME == (*unique_objects)[j].getNAME()) {
            validART = false;
            break;
          }
        }

        if (!validART) {
          randIndex = rand() % this->possible_items->size();
          potentialItem = (*this->possible_items)[randIndex];
          continue; // recheck artifact
        }
      }

      int rarity = rand() % 100 + 1;
      if (rarity < (int)potentialItem.RRTY) {
        overallValidity = true;
      } else {
        // Try a fallback item
        randIndex = (randIndex + 1) % this->possible_items->size();
        potentialItem = (*this->possible_items)[randIndex];
      }
    }

    if (attempts >= 1000) {
      fprintf(stderr, "Failed to generate a valid item after 1000 attempts.\n");
      continue;
    }

    Objects* newItem = new Objects();

    int roomAttempts = 0;
    do {
      room = rand_range(1, this->num_rooms - 1);
      p[dim_y] = rand_range(this->rooms[room].position[dim_y],
                            this->rooms[room].position[dim_y] + this->rooms[room].size[dim_y] - 1);
      p[dim_x] = rand_range(this->rooms[room].position[dim_x],
                            this->rooms[room].position[dim_x] + this->rooms[room].size[dim_x] - 1);
    } while (this->game_objects[p[dim_y]][p[dim_x]] && roomAttempts++ < 100);

    if (roomAttempts >= 100) {
      fprintf(stderr, "Could not find space to place object.\n");
      delete newItem;
      continue;
    }

    newItem->position[dim_y] = p[dim_y];
    newItem->position[dim_x] = p[dim_x];
    this->game_objects[p[dim_y]][p[dim_x]] = newItem;


    newItem->setNAME(potentialItem.NAME);
    newItem->setCOLOR(potentialItem.COLOR);
    newItem->setTYPE(potentialItem.TYPE);

    newItem->setWEIGHT(potentialItem.WEIGHT);
    newItem->setHIT(potentialItem.HIT);
    newItem->setDAM(potentialItem.DAM);
    newItem->setATTR(potentialItem.ATTR);
    newItem->setVAL(potentialItem.VAL);
    newItem->setDODGE(potentialItem.DODGE);
    newItem->setDEF(potentialItem.DEF);
    newItem->setSPEED(potentialItem.SPEED);

    newItem->setDESC(potentialItem.DESC);
    newItem->setRRTY(potentialItem.RRTY);
    newItem->setART(potentialItem.ART);

    if (potentialItem.ART) {
      unique_objects->push_back(*newItem);
    }
  }
}


void dungeon_t::gen_monsters() {
  uint32_t i;
  int j;
  uint32_t room;
  pair_t p;

  this->num_monsters = std::min(this->max_monsters, static_cast<uint16_t>(this->max_monster_cells()));

  for(i = 0; i < this->num_monsters; i++){
    //Getting a monster
    monsterDescription_t md;
    bool generatedValid = false;

    while (!generatedValid){
      int randIndex = rand() % this->possible_monsters->size();
      md = (*this->possible_monsters)[randIndex];

      //Check extinct_monsters
      bool extinct = false; 
      for(j = 0; j < (int)extinct_monsters->size(); j++){
        if(md.NAME == (*extinct_monsters)[j]->getNAME()){
          extinct = true;
        }
      }

      if(!extinct){
        //Generate rarity check
        int rarity = rand() % 100 + 1;
        if(rarity < (int)md.RRTY){
          generatedValid = true;
        }
      }

    }

    npc_t *npc = new npc_t();
    
    do {
      room = rand_range(1, this->num_rooms - 1);
      p[dim_y] = rand_range(this->rooms[room].position[dim_y],
                            this->rooms[room].position[dim_y] + this->rooms[room].size[dim_y] - 1);
      p[dim_x] = rand_range(this->rooms[room].position[dim_x],
                            this->rooms[room].position[dim_x] + this->rooms[room].size[dim_x] - 1);
    } while (this->character[p[dim_y]][p[dim_x]]);

    npc->position[dim_y] = p[dim_y];
    npc->position[dim_x] = p[dim_x];
    this->character[p[dim_y]][p[dim_x]] = npc;

    npc->setNAME(md.NAME);
    npc->setDESC(md.DESC);
    npc->setCOLOR(md.COLOR);
    npc->setSPEED(md.SPEED);
    npc->setABIL(md.ABIL);
    npc->setHP(md.HP);
    npc->setDAM(md.DAM);
    npc->setSYMB(md.SYMB);
    npc->setRRTY(md.RRTY);
    

    npc->symbol = md.SYMB;
    npc->alive = 1;
    npc->sequence_number = ++this->character_sequence_number;
    npc->npc = static_cast<npc_t*>(npc); 
    npc->pc = nullptr;
    npc->have_seen_pc = 0;
    npc->kills[kill_direct] = npc->kills[kill_avenged] = 0;


    heap_insert(&this->events, new_event(this, event_character_turn, npc, 0));



  }
}

// void dungeon_t::gen_monsters() {
//   uint32_t i;
//   character_t *m;
//   uint32_t room;
//   pair_t p;
//   const static char symbol[] = "0123456789abcdef";
  
  

//   this->num_monsters = std::min(this->max_monsters, static_cast<uint16_t>(this->max_monster_cells()));


//   for (i = 0; i < this->num_monsters; i++) {
//       m = new character_t();  // Allocate memory for character
//       //fprintf(stderr, "Allocating character %p\n", (void*)m);

//       do {
//           room = rand_range(1, this->num_rooms - 1);
//           p[dim_y] = rand_range(this->rooms[room].position[dim_y],
//                                 (this->rooms[room].position[dim_y] +
//                                  this->rooms[room].size[dim_y] - 1));
//           p[dim_x] = rand_range(this->rooms[room].position[dim_x],
//                                 (this->rooms[room].position[dim_x] +
//                                  this->rooms[room].size[dim_x] - 1));
//       } while (this->character[p[dim_y]][p[dim_x]]);

//       m->position[dim_y] = p[dim_y];
//       m->position[dim_x] = p[dim_x];
//       this->character[p[dim_y]][p[dim_x]] = m;

//       m->speed = rand_range(NPC_MIN_SPEED, NPC_MAX_SPEED);
//       m->alive = 1;
//       m->sequence_number = ++this->character_sequence_number;

//       m->pc = NULL;

//       m->npc = new npc_t();
//       if (!m->npc) {
//           fprintf(stderr, "Failed to allocate memory for NPC.\n");
//           exit(1);
//       }
//       fprintf(stderr, "NPC allocated %p\n", (void*)m->npc);

//       m->npc->characteristics = rand() % 15 + 0;

//       if (m->npc->characteristics >= 16) {
//           fprintf(stderr, "NPC characteristics out of range: %d\n", m->npc->characteristics);
//           m->npc->characteristics = 0;  // Default to '0' if out of range
//       }

//       m->symbol = static_cast<char>(symbol[m->npc->characteristics]);
//       fprintf(stderr, "Symbol assigned: %c\n", m->symbol);

//       //mvprintw(0, 0, "Assigned NPC characteristics: %d  ", m->npc->characteristics);
//       //refresh();

//       m->npc->have_seen_pc = 0;
//       m->kills[kill_direct] = m->kills[kill_avenged] = 0;

//       heap_insert(&this->events, new_event(this, event_character_turn, m, 0));
//   }
// }

