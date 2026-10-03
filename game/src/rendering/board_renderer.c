#include "board_renderer.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>

#include "board.h"
#include "game_constants.h"
#include "graphics.h"
#include "hex_colors.h"
#include "hex_grid.h"
#include "text.h"

static void hexagon_points(point_t centre, double radius,
                           SDL_Point points[HEX_CORNER_COUNT]) {
  for (int corner = 0; corner < HEX_CORNER_COUNT; ++corner) {
    point_t p = hex_corner(centre, radius, corner);
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

static void render_empty_cell(const graphics_context_ptr graphics_context,
                              point_t centre, double radius, bool hovered) {
  SDL_Point points[HEX_CORNER_COUNT];
  hexagon_points(centre, radius, points);
  if (hovered) {
    draw_filled_polygon(graphics_context, points, HEX_CORNER_COUNT,
                        HOVERED_CELL_FILL_COLOR);
  }
  draw_hexagon_outline(
      graphics_context, points,
      hovered ? HOVERED_CELL_BORDER_COLOR : EMPTY_CELL_BORDER_COLOR);
}

// Largest whole scale at which the text fits in a box
static int fitting_text_scale(const char* text, double max_width,
                              double max_height) {
  int scale = 1;
  while (true) {
    text_dimensions_t next = calculate_text_dimensions(text, scale + 1);
    if (next.width > max_width || next.height > max_height) {
      return scale;
    }
    ++scale;
  }
}

static void render_number(const graphics_context_ptr graphics_context,
                          point_t centre, double radius, int value,
                          color_t color) {
  char text[16];
  snprintf(text, sizeof(text), "%d", value);
  int scale = fitting_text_scale(text, radius * HEX_NUMBER_WIDTH_FRACTION,
                                 radius * HEX_NUMBER_HEIGHT_FRACTION);
  text_dimensions_t dimensions = calculate_text_dimensions(text, scale);
  // Text is written upwards from its bottom-left corner
  write_text(graphics_context, text,
             point(centre.x - dimensions.width / 2.0,
                   centre.y + dimensions.height / 2.0),
             scale, color);
}

static void render_hexagon(const graphics_context_ptr graphics_context,
                           point_t centre, double radius, int value) {
  color_t border = hex_border_color(value);
  SDL_Point points[HEX_CORNER_COUNT];
  hexagon_points(centre, radius, points);
  draw_filled_polygon(graphics_context, points, HEX_CORNER_COUNT, border);
  double inner_radius = radius * (1 - HEX_BORDER_THICKNESS_FRACTION);
  hexagon_points(centre, inner_radius, points);
  draw_filled_polygon(graphics_context, points, HEX_CORNER_COUNT,
                      HEX_FILL_COLOR);
  render_number(graphics_context, centre, inner_radius, value, border);
}

void render_board(const graphics_context_ptr graphics_context,
                  const hex_grid_t* grid, const board_t* board,
                  const cell_t* hovered) {
  double radius = grid->radius * HEX_DRAWN_RADIUS_FRACTION;
  for (int row = 0; row < grid->rows; ++row) {
    for (int col = 0; col < grid->cols; ++col) {
      cell_t cell = {col, row};
      point_t centre = hex_cell_centre(grid, cell);
      int value = board_value(board, cell);
      if (value == EMPTY_CELL) {
        bool is_hovered = hovered && hovered->col == col && hovered->row == row;
        render_empty_cell(graphics_context, centre, radius, is_hovered);
      } else {
        render_hexagon(graphics_context, centre, radius, value);
      }
    }
  }
}
