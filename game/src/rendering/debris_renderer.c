#include "debris_renderer.h"

#include <SDL.h>
#include <math.h>

#include "debris.h"
#include "graphics.h"

void render_debris(const graphics_context_ptr graphics_context,
                   const debris_t* debris) {
  for (int i = 0; i < debris->capacity; ++i) {
    const debris_piece_t* piece = &debris->pieces[i];
    if (!piece->active) {
      continue;
    }
    point_t corners[DEBRIS_PIECE_CORNERS];
    debris_piece_corners(piece, corners);
    SDL_Point points[DEBRIS_PIECE_CORNERS];
    for (int corner = 0; corner < DEBRIS_PIECE_CORNERS; ++corner) {
      points[corner].x = (int)lround(corners[corner].x);
      points[corner].y = (int)lround(corners[corner].y);
    }
    draw_filled_polygon(graphics_context, points, DEBRIS_PIECE_CORNERS,
                        piece->color);
  }
}
