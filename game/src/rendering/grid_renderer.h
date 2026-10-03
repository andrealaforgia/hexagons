/**
 * @file grid_renderer.h
 * @brief Draws the hexagonal playfield
 */

#ifndef GAME_SRC_RENDERING_GRID_RENDERER_H_
#define GAME_SRC_RENDERING_GRID_RENDERER_H_

#include "graphics.h"
#include "hex_grid.h"

/**
 * @brief Draw the outline of every cell, highlighting one of them
 * @param hovered Cell to highlight, or NULL for none
 */
void render_grid(const graphics_context_ptr graphics_context,
                 const hex_grid_t* grid, const cell_t* hovered);

#endif  // GAME_SRC_RENDERING_GRID_RENDERER_H_
