#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "game_settings.h"
#include "hex_grid.h"
#include "playing_stage.h"
#include "stage.h"
#include "test_allocator.h"

static game_t test_game(void) {
  game_t game = {0};
  game.settings = init_game_settings(false, false, 0, 0, WINDOWED, 60);
  game.graphics_context.screen_width = 1440;
  game.graphics_context.screen_height = 900;
  game.graphics_context.screen_center = point(720, 450);
  return game;
}

static void stages_release_every_allocation(void) {
  game_t game = test_game();
  for (int replay = 0; replay < 10; ++replay) {
    playing_stage_state_ptr state = create_playing_stage(&game);
    assert(state);
    destroy_playing_stage(state);
    assert(outstanding_allocations() == 0);
  }
}

static void stage_creation_cleans_up_on_failure(void) {
  game_t game = test_game();
  bool succeeded = false;
  for (int allocation = 0; allocation < 64; ++allocation) {
    fail_allocation_after(allocation);
    playing_stage_state_ptr state = create_playing_stage(&game);
    if (state) {
      destroy_playing_stage(state);
      assert(outstanding_allocations() == 0);
      succeeded = true;
      break;
    }
    assert(outstanding_allocations() == 0);
  }
  assert(succeeded);
  fail_allocation_after(-1);
}

static void stage_factories_handle_allocation_failure(void) {
  fail_allocation_after(0);
  assert(create_playing_stage_instance() == NULL);
  fail_allocation_after(-1);
  stage_ptr stage = create_playing_stage_instance();
  assert(stage);
  destroy_stage(stage);
  assert(outstanding_allocations() == 0);
}

static void every_cell_centre_maps_back_to_its_cell(void) {
  hex_grid_t grid = create_hex_grid(1440, 900, 37.5);
  assert(hex_grid_cell_count(&grid) > 0);
  // Stay inside the hexagon: its inner radius is sqrt(3) / 2 of the outer one.
  double reach = grid.radius * 0.85;
  for (int row = 0; row < grid.rows; ++row) {
    for (int col = 0; col < grid.cols; ++col) {
      cell_t cell = {col, row};
      point_t centre = hex_cell_centre(&grid, cell);
      for (int side = -1; side < 6; ++side) {
        double angle = side * M_PI / 3;
        double distance = side < 0 ? 0 : reach;
        cell_t found = {-1, -1};
        assert(hex_cell_at(&grid, centre.x + distance * cos(angle),
                           centre.y + distance * sin(angle), &found));
        assert(found.col == col && found.row == row);
      }
    }
  }
}

static void grid_fills_the_screen_without_overflowing(void) {
  const int sizes[][2] = {{1440, 900}, {1920, 1080}, {3024, 1964}, {800, 600}};
  for (int i = 0; i < 4; ++i) {
    int width = sizes[i][0], height = sizes[i][1];
    double radius = height / 24.0;
    hex_grid_t grid = create_hex_grid(width, height, radius);
    double hex_width = sqrt(3) * radius;
    double left = width, right = 0, top = height, bottom = 0;
    for (int row = 0; row < grid.rows; ++row) {
      for (int col = 0; col < grid.cols; ++col) {
        cell_t cell = {col, row};
        point_t centre = hex_cell_centre(&grid, cell);
        left = fmin(left, centre.x - hex_width / 2);
        right = fmax(right, centre.x + hex_width / 2);
        top = fmin(top, centre.y - radius);
        bottom = fmax(bottom, centre.y + radius);
      }
    }
    // Every hexagon is wholly on screen
    assert(left >= -1e-6 && right <= width + 1e-6);
    assert(top >= -1e-6 && bottom <= height + 1e-6);
    // There is no room for one more column or row
    assert(width - (right - left) < hex_width);
    assert(height - (bottom - top) < 1.5 * radius);
    // The margins are even
    assert(fabs(left - (width - right)) < 1e-6);
    assert(fabs(top - (height - bottom)) < 1e-6);
  }
}

static void points_outside_every_hexagon_belong_to_no_cell(void) {
  hex_grid_t grid = create_hex_grid(1440, 900, 37.5);
  cell_t cell = {7, 7};
  assert(!hex_cell_at(&grid, -500, -500, &cell));
  assert(!hex_cell_at(&grid, 5000, 450, &cell));
  assert(!hex_cell_at(&grid, 720, 5000, &cell));
  // The top-left corner of the screen lies above the first hexagon's slope
  assert(!hex_cell_at(&grid, 0, 0, &cell));
  assert(cell.col == 7 && cell.row == 7);
}

static void pointing_highlights_the_cell_under_the_mouse(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  assert(!state->has_hovered_cell);
  cell_t cell = {3, 2};
  point_t centre = hex_cell_centre(&state->grid, cell);
  point_playing_stage_at(state, centre.x, centre.y);
  assert(state->has_hovered_cell);
  assert(state->hovered_cell.col == 3 && state->hovered_cell.row == 2);
  point_playing_stage_at(state, -500, -500);
  assert(!state->has_hovered_cell);
  destroy_playing_stage(state);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  if (!strcmp(argv[1], "lifecycle"))
    stages_release_every_allocation();
  else if (!strcmp(argv[1], "allocation"))
    stage_creation_cleans_up_on_failure();
  else if (!strcmp(argv[1], "factories"))
    stage_factories_handle_allocation_failure();
  else if (!strcmp(argv[1], "roundtrip"))
    every_cell_centre_maps_back_to_its_cell();
  else if (!strcmp(argv[1], "fill"))
    grid_fills_the_screen_without_overflowing();
  else if (!strcmp(argv[1], "outside"))
    points_outside_every_hexagon_belong_to_no_cell();
  else if (!strcmp(argv[1], "hover"))
    pointing_highlights_the_cell_under_the_mouse();
  else
    return 1;
  return 0;
}
