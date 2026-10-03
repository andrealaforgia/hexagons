#include "grid_renderer.h"

#include <SDL.h>
#include <math.h>

#include "game_constants.h"
#include "graphics.h"
#include "hex_grid.h"

static void hexagon_points(const hex_grid_t* grid, cell_t cell,
                           SDL_Point points[HEX_CORNER_COUNT]) {
  point_t centre = hex_cell_centre(grid, cell);
  for (int corner = 0; corner < HEX_CORNER_COUNT; ++corner) {
    point_t p =
        hex_corner(centre, grid->radius * HEX_DRAWN_RADIUS_FRACTION, corner);
    points[corner].x = (int)lround(p.x);
    points[corner].y = (int)lround(p.y);
  }
}

static void draw_hexagon_outline(const graphics_context_ptr graphics_context,
                                 const SDL_Point points[HEX_CORNER_COUNT],
                                 color_t color) {
  for (int corner = 0; corner < HEX_CORNER_COUNT; ++corner) {
    const SDL_Point* from = &points[corner];
    const SDL_Point* to = &points[(corner + 1) % HEX_CORNER_COUNT];
    draw_line(graphics_context, from->x, from->y, to->x, to->y, color);
  }
}

void render_grid(const graphics_context_ptr graphics_context,
                 const hex_grid_t* grid, const cell_t* hovered) {
  SDL_Point points[HEX_CORNER_COUNT];
  for (int row = 0; row < grid->rows; ++row) {
    for (int col = 0; col < grid->cols; ++col) {
      cell_t cell = {col, row};
      hexagon_points(grid, cell, points);
      draw_hexagon_outline(graphics_context, points, EMPTY_CELL_BORDER_COLOR);
    }
  }
  if (hovered) {
    hexagon_points(grid, *hovered, points);
    draw_filled_polygon(graphics_context, points, HEX_CORNER_COUNT,
                        HOVERED_CELL_FILL_COLOR);
    draw_hexagon_outline(graphics_context, points, HOVERED_CELL_BORDER_COLOR);
  }
}
