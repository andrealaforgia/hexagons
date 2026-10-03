/**
 * @file debris.h
 * @brief Pieces of exploded hexagons
 *
 * A hexagon bursts into its six wedges, which are thrown upwards, spin, and
 * fall under gravity until they drop off the bottom of the screen. Purely
 * for show: the pieces have no effect on the game.
 */

#ifndef GAME_SRC_EFFECTS_DEBRIS_H_
#define GAME_SRC_EFFECTS_DEBRIS_H_

#include <stdbool.h>

#include "color.h"
#include "geometry.h"
#include "random_source.h"

#define DEBRIS_PIECES_PER_HEXAGON 6
#define DEBRIS_PIECE_CORNERS 3

typedef struct {
  bool active;
  point_t position;   // Of the centre of the wedge
  vector_t velocity;  // In pixels per second
  double angle;       // How far the piece has turned, in radians
  double spin;        // In radians per second
  double radius;      // Of the hexagon it came from
  int wedge;          // Which sixth of the hexagon it is
  color_t color;
} debris_piece_t;

typedef struct {
  debris_piece_t* pieces;
  int capacity;
} debris_t;

/** @return false if the pieces could not be allocated */
bool init_debris(debris_t* debris, int capacity);
void destroy_debris(debris_t* debris);

/**
 * @brief Blow up a hexagon
 *
 * Pieces for which there is no room are dropped.
 */
void burst_hexagon(debris_t* debris, random_source_t* random, point_t centre,
                   double radius, color_t color);

/**
 * @brief Let the pieces fly
 * @param delta_time Time since the last call, in frames at 60 per second
 * @param floor_y Bottom of the screen; pieces wholly below it are discarded
 */
void advance_debris(debris_t* debris, double delta_time, double floor_y);

int debris_piece_count(const debris_t* debris);

/** @brief Where on screen the three corners of a piece are */
void debris_piece_corners(const debris_piece_t* piece,
                          point_t corners[DEBRIS_PIECE_CORNERS]);

#endif  // GAME_SRC_EFFECTS_DEBRIS_H_
