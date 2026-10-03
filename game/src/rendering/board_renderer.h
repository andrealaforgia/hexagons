/**
 * @file board_renderer.h
 * @brief Draws the hexagonal playfield and the hexagons on it
 */

#ifndef GAME_SRC_RENDERING_BOARD_RENDERER_H_
#define GAME_SRC_RENDERING_BOARD_RENDERER_H_

#include "board.h"
#include "graphics.h"
#include "hex_grid.h"
#include "number_text.h"

// A hexagon on its way to the cell it was moved to
typedef struct {
  cell_t destination;
  point_t position;  // Where it is on screen right now
} travelling_hexagon_t;

/**
 * @brief Draw every cell: an outline when empty, a solid grey hexagon for a
 * wall, a numbered hexagon otherwise
 *
 * The selected hexagon is filled with a light tone of its border colour.
 * @param hovered Cell to highlight, or NULL for none
 * @param travelling Hexagon to draw on its way rather than on its cell, or
 * NULL for none
 * @param numbers Font for the numbers; they are line-drawn if it is not loaded
 */
void render_board(const graphics_context_ptr graphics_context,
                  const hex_grid_t* grid, const board_t* board,
                  const cell_t* hovered, const travelling_hexagon_t* travelling,
                  number_text_t* numbers);

#endif  // GAME_SRC_RENDERING_BOARD_RENDERER_H_
