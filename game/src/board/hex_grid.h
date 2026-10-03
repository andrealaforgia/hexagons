/**
 * @file hex_grid.h
 * @brief Geometry of the hexagonal playfield
 *
 * Lays out pointy-top hexagons in rows, with odd rows shifted half a
 * hexagon to the right, so that the grid fills a screen. Converts between
 * cells and screen positions. Knows nothing about what the cells contain.
 */

#ifndef GAME_SRC_BOARD_HEX_GRID_H_
#define GAME_SRC_BOARD_HEX_GRID_H_

#include <stdbool.h>

#include "geometry.h"

#define HEX_CORNER_COUNT 6

typedef struct {
  int col;
  int row;
} cell_t;

typedef struct {
  int cols;
  int rows;
  double radius;   // Centre to corner
  point_t origin;  // Top-left of the area the grid covers
} hex_grid_t;

/**
 * @brief Lay out as many hexagons of the given radius as fit on the screen
 *
 * The grid is centred, leaving even margins on opposite sides.
 */
hex_grid_t create_hex_grid(int screen_width, int screen_height, double radius);

int hex_grid_cell_count(const hex_grid_t* grid);
bool hex_grid_contains(const hex_grid_t* grid, cell_t cell);

point_t hex_cell_centre(const hex_grid_t* grid, cell_t cell);

/**
 * @brief Corner of a hexagon, counted clockwise on screen from the top right
 */
point_t hex_corner(point_t centre, double radius, int corner);

/**
 * @brief Find the cell whose hexagon contains a screen position
 * @param cell Set to the cell found; left untouched when there is none
 * @return true if the position lies inside a hexagon of the grid
 */
bool hex_cell_at(const hex_grid_t* grid, double x, double y, cell_t* cell);

#endif  // GAME_SRC_BOARD_HEX_GRID_H_
