#include "debris.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "game_constants.h"
#include "geometry.h"
#include "hex_grid.h"
#include "random_source.h"

// Speeds are in hexagon radii, so the effect looks the same on any screen
#define LAUNCH_SPEED_MIN 18.0  // Upwards, radii per second
#define LAUNCH_SPEED_MAX 30.0
#define OUTWARD_SPEED 5.0   // Away from the centre of the hexagon
#define SIDEWAYS_SPEED 6.0  // Random drift to either side
#define GRAVITY 70.0        // Radii per second squared
#define MAX_SPIN 8.0        // Radians per second

bool init_debris(debris_t* debris, int capacity) {
  debris->capacity = capacity;
  debris->pieces = calloc((size_t)capacity, sizeof(debris_piece_t));
  return debris->pieces != NULL;
}

void destroy_debris(debris_t* debris) {
  free(debris->pieces);
  debris->pieces = NULL;
  debris->capacity = 0;
}

// Random number between two bounds
static double random_between(random_source_t* random, double low, double high) {
  return low + (high - low) * random_below(random, 1001) / 1000.0;
}

// The corners of a wedge relative to the centre of its hexagon
static void wedge_corners(double radius, int wedge,
                          point_t corners[DEBRIS_PIECE_CORNERS]) {
  point_t centre = point(0, 0);
  corners[0] = centre;
  corners[1] = hex_corner(centre, radius, wedge);
  corners[2] = hex_corner(centre, radius, (wedge + 1) % HEX_CORNER_COUNT);
}

static point_t wedge_centre(double radius, int wedge) {
  point_t corners[DEBRIS_PIECE_CORNERS];
  wedge_corners(radius, wedge, corners);
  return point((corners[1].x + corners[2].x) / 3,
               (corners[1].y + corners[2].y) / 3);
}

static debris_piece_t* free_piece(debris_t* debris) {
  for (int i = 0; i < debris->capacity; ++i) {
    if (!debris->pieces[i].active) {
      return &debris->pieces[i];
    }
  }
  return NULL;
}

void burst_hexagon(debris_t* debris, random_source_t* random, point_t centre,
                   double radius, color_t color) {
  for (int wedge = 0; wedge < DEBRIS_PIECES_PER_HEXAGON; ++wedge) {
    debris_piece_t* piece = free_piece(debris);
    if (!piece) {
      return;
    }
    point_t offset = wedge_centre(radius, wedge);
    double distance = hypot(offset.x, offset.y);
    double launch = random_between(random, LAUNCH_SPEED_MIN, LAUNCH_SPEED_MAX);
    double drift = random_between(random, -SIDEWAYS_SPEED, SIDEWAYS_SPEED);
    piece->active = true;
    piece->position = point(centre.x + offset.x, centre.y + offset.y);
    piece->velocity =
        vector((offset.x / distance * OUTWARD_SPEED + drift) * radius,
               -launch * radius);
    piece->angle = 0;
    piece->spin = random_between(random, -MAX_SPIN, MAX_SPIN);
    piece->radius = radius;
    piece->wedge = wedge;
    piece->color = color;
  }
}

void advance_debris(debris_t* debris, double delta_time, double floor_y) {
  if (!isfinite(delta_time) || delta_time <= 0) {
    return;
  }
  double seconds = delta_time / BASELINE_FPS;
  for (int i = 0; i < debris->capacity; ++i) {
    debris_piece_t* piece = &debris->pieces[i];
    if (!piece->active) {
      continue;
    }
    piece->velocity.y += GRAVITY * piece->radius * seconds;
    piece->position.x += piece->velocity.x * seconds;
    piece->position.y += piece->velocity.y * seconds;
    piece->angle += piece->spin * seconds;
    if (piece->position.y - piece->radius > floor_y) {
      piece->active = false;
    }
  }
}

int debris_piece_count(const debris_t* debris) {
  int count = 0;
  for (int i = 0; i < debris->capacity; ++i) {
    if (debris->pieces[i].active) {
      ++count;
    }
  }
  return count;
}

void clear_debris(debris_t* debris) {
  for (int i = 0; i < debris->capacity; ++i) {
    debris->pieces[i].active = false;
  }
}

void debris_piece_corners(const debris_piece_t* piece,
                          point_t corners[DEBRIS_PIECE_CORNERS]) {
  wedge_corners(piece->radius, piece->wedge, corners);
  point_t centre = wedge_centre(piece->radius, piece->wedge);
  double cosine = cos(piece->angle), sine = sin(piece->angle);
  for (int i = 0; i < DEBRIS_PIECE_CORNERS; ++i) {
    // Turn the corner about the centre of the wedge
    double x = corners[i].x - centre.x, y = corners[i].y - centre.y;
    corners[i] = point(piece->position.x + x * cosine - y * sine,
                       piece->position.y + x * sine + y * cosine);
  }
}
