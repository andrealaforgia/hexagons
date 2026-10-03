/**
 * @file debris_renderer.h
 * @brief Draws the pieces of exploded hexagons
 */

#ifndef GAME_SRC_RENDERING_DEBRIS_RENDERER_H_
#define GAME_SRC_RENDERING_DEBRIS_RENDERER_H_

#include "debris.h"
#include "graphics.h"

void render_debris(const graphics_context_ptr graphics_context,
                   const debris_t* debris);

#endif  // GAME_SRC_RENDERING_DEBRIS_RENDERER_H_
