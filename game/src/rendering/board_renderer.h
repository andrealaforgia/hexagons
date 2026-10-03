/**
 * @file board_renderer.h
 * @brief Draws the hexagonal playfield and the hexagons on it
 */

#ifndef GAME_SRC_RENDERING_BOARD_RENDERER_H_
#define GAME_SRC_RENDERING_BOARD_RENDERER_H_

#include "board.h"
#include "graphics.h"
#include "hex_grid.h"

/**
 * @brief Draw every cell: an outline when empty, a numbered hexagon otherwise
 *
 * The selected hexagon is filled with a light tone of its border colour.
 * @param hovered Cell to highlight, or NULL for none
 */
void render_board(const graphics_context_ptr graphics_context,
                  const hex_grid_t* grid, const board_t* board,
                  const cell_t* hovered);

#endif  // GAME_SRC_RENDERING_BOARD_RENDERER_H_
