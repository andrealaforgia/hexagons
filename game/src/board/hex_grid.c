#include "hex_grid.h"

#include <math.h>
#include <stdbool.h>

#include "geometry.h"

static double hex_width(double radius) { return sqrt(3) * radius; }

static double row_spacing(double radius) { return 1.5 * radius; }

static int max_int(int a, int b) { return a > b ? a : b; }

hex_grid_t create_hex_grid(int screen_width, int screen_height, double radius) {
  hex_grid_t grid = {0};
  grid.radius = radius;
  double width = hex_width(radius);
  grid.rows = max_int(
      0, (int)floor((screen_height - 2 * radius) / row_spacing(radius)) + 1);
  // Odd rows are shifted right by half a hexagon
  double shift = grid.rows > 1 ? width / 2 : 0;
  grid.cols = max_int(0, (int)floor((screen_width - shift) / width));
  double covered_width = grid.cols * width + shift;
  double covered_height = (grid.rows - 1) * row_spacing(radius) + 2 * radius;
  grid.origin = point((screen_width - covered_width) / 2,
                      (screen_height - covered_height) / 2);
  return grid;
}

int hex_grid_cell_count(const hex_grid_t* grid) {
  return grid->cols * grid->rows;
}

bool hex_grid_contains(const hex_grid_t* grid, cell_t cell) {
  return cell.col >= 0 && cell.col < grid->cols && cell.row >= 0 &&
         cell.row < grid->rows;
}

point_t hex_cell_centre(const hex_grid_t* grid, cell_t cell) {
  double width = hex_width(grid->radius);
  double shift = (cell.row & 1) ? width / 2 : 0;
  return point(
      grid->origin.x + width / 2 + cell.col * width + shift,
      grid->origin.y + grid->radius + cell.row * row_spacing(grid->radius));
}

point_t hex_corner(point_t centre, double radius, int corner) {
  double angle = (corner * 60 - 30) * M_PI / 180;
  return point(centre.x + radius * cos(angle), centre.y + radius * sin(angle));
}

bool hex_cell_at(const hex_grid_t* grid, double x, double y, cell_t* cell) {
  // Position relative to the centre of cell (0, 0), in fractional axial
  // coordinates
  double dx = x - grid->origin.x - hex_width(grid->radius) / 2;
  double dy = y - grid->origin.y - grid->radius;
  double q = (sqrt(3) / 3 * dx - dy / 3) / grid->radius;
  double r = (2.0 / 3 * dy) / grid->radius;

  // Round to the nearest hexagon in cube coordinates, fixing the component
  // with the largest rounding error so that the three still sum to zero
  double s = -q - r;
  double rounded_q = round(q), rounded_r = round(r), rounded_s = round(s);
  double q_error = fabs(rounded_q - q);
  double r_error = fabs(rounded_r - r);
  double s_error = fabs(rounded_s - s);
  if (q_error > r_error && q_error > s_error) {
    rounded_q = -rounded_r - rounded_s;
  } else if (r_error > s_error) {
    rounded_r = -rounded_q - rounded_s;
  }

  int row = (int)rounded_r;
  cell_t found = {(int)rounded_q + (row - (row & 1)) / 2, row};
  if (!hex_grid_contains(grid, found)) {
    return false;
  }
  *cell = found;
  return true;
}
