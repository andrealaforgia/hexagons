#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "game.h"
#include "game_settings.h"
#include "hex_colors.h"
#include "hex_grid.h"
#include "playing_stage.h"
#include "random_source.h"
#include "stage.h"
#include "test_allocator.h"

static game_t test_game(void) {
  game_t game = {0};
  game.settings = init_game_settings(false, false, 0, 0, WINDOWED, 60);
  game.graphics_context.screen_width = 1440;
  game.graphics_context.screen_height = 900;
  game.graphics_context.screen_center = point(720, 450);
  game.seed = 1;
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

static void a_new_board_holds_a_tenth_of_its_cells(void) {
  const int sizes[][2] = {{21, 15}, {10, 10}, {7, 3}, {1, 1}};
  for (int i = 0; i < 4; ++i) {
    board_t board;
    assert(init_board(&board, sizes[i][0], sizes[i][1]));
    int cells = sizes[i][0] * sizes[i][1];
    assert(board_hexagon_count(&board) == 0);
    random_source_t random = create_random_source(42);
    int expected = (int)lround(cells / 10.0);
    assert(initial_hexagon_count(cells) == expected);
    assert(populate_board(&board, &random, expected) == expected);
    // Hexagons on distinct cells: the count of occupied cells is the count
    // of hexagons placed
    assert(board_hexagon_count(&board) == expected);
    destroy_board(&board);
  }
  assert(outstanding_allocations() == 0);
}

static void new_hexagons_are_numbered_from_one_to_eight(void) {
  board_t board;
  assert(init_board(&board, 40, 40));
  random_source_t random = create_random_source(7);
  populate_board(&board, &random, 800);
  int seen[MAX_NEW_HEXAGON_VALUE + 1] = {0};
  for (int row = 0; row < 40; ++row) {
    for (int col = 0; col < 40; ++col) {
      cell_t cell = {col, row};
      int value = board_value(&board, cell);
      assert(value >= 0 && value <= MAX_NEW_HEXAGON_VALUE);
      seen[value]++;
    }
  }
  assert(seen[0] == 800);
  for (int value = 1; value <= MAX_NEW_HEXAGON_VALUE; ++value) {
    // Roughly even: 100 expected of each
    assert(seen[value] > 50 && seen[value] < 150);
  }
  destroy_board(&board);
}

static bool boards_match(const board_t* a, const board_t* b) {
  for (int row = 0; row < a->rows; ++row) {
    for (int col = 0; col < a->cols; ++col) {
      cell_t cell = {col, row};
      if (board_value(a, cell) != board_value(b, cell)) return false;
    }
  }
  return true;
}

static void the_same_seed_gives_the_same_board(void) {
  board_t first, second, third;
  assert(init_board(&first, 21, 15) && init_board(&second, 21, 15) &&
         init_board(&third, 21, 15));
  random_source_t a = create_random_source(1234);
  random_source_t b = create_random_source(1234);
  random_source_t c = create_random_source(1235);
  populate_board(&first, &a, 32);
  populate_board(&second, &b, 32);
  populate_board(&third, &c, 32);
  assert(boards_match(&first, &second));
  assert(!boards_match(&first, &third));
  destroy_board(&first);
  destroy_board(&second);
  destroy_board(&third);
}

static void populating_stops_when_the_board_is_full(void) {
  board_t board;
  assert(init_board(&board, 4, 3));
  random_source_t random = create_random_source(3);
  assert(populate_board(&board, &random, 10) == 10);
  assert(populate_board(&board, &random, 10) == 2);
  assert(populate_board(&board, &random, 10) == 0);
  assert(board_hexagon_count(&board) == 12);
  destroy_board(&board);
}

static void cells_outside_the_board_are_empty_and_cannot_be_set(void) {
  board_t board;
  assert(init_board(&board, 4, 3));
  cell_t inside = {3, 2}, outside = {4, 2}, negative = {-1, 0};
  set_board_value(&board, inside, 5);
  set_board_value(&board, outside, 5);
  set_board_value(&board, negative, 5);
  assert(board_value(&board, inside) == 5);
  assert(board_value(&board, outside) == 0);
  assert(board_value(&board, negative) == 0);
  assert(board_hexagon_count(&board) == 1);
  destroy_board(&board);
}

static void each_number_has_its_own_border_colour(void) {
  for (int value = 1; value <= MAX_NEW_HEXAGON_VALUE; ++value) {
    assert(hex_border_color(value) == hex_border_color(value));
    assert(hex_border_color(value) != COLOR_BLACK);
    for (int other = value + 1; other <= MAX_NEW_HEXAGON_VALUE; ++other) {
      color_t a = hex_border_color(value), b = hex_border_color(other);
      // Tell them apart at a glance, not just by one shade
      int difference = abs(R(a) - R(b)) + abs(G(a) - G(b)) + abs(B(a) - B(b));
      assert(difference >= 60);
    }
  }
  // Sums of merges get a colour too, and bright enough to see on black
  for (int value = 1; value <= 1024; ++value) {
    color_t color = hex_border_color(value);
    assert(R(color) + G(color) + B(color) >= 200);
  }
}

static void the_game_starts_with_a_tenth_of_the_cells_filled(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  int cells = hex_grid_cell_count(&state->grid);
  assert(state->board.cols == state->grid.cols);
  assert(state->board.rows == state->grid.rows);
  assert(board_hexagon_count(&state->board) == (int)lround(cells / 10.0));
  destroy_playing_stage(state);
  assert(outstanding_allocations() == 0);
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
  else if (!strcmp(argv[1], "population"))
    a_new_board_holds_a_tenth_of_its_cells();
  else if (!strcmp(argv[1], "values"))
    new_hexagons_are_numbered_from_one_to_eight();
  else if (!strcmp(argv[1], "seed"))
    the_same_seed_gives_the_same_board();
  else if (!strcmp(argv[1], "full"))
    populating_stops_when_the_board_is_full();
  else if (!strcmp(argv[1], "bounds"))
    cells_outside_the_board_are_empty_and_cannot_be_set();
  else if (!strcmp(argv[1], "colours"))
    each_number_has_its_own_border_colour();
  else if (!strcmp(argv[1], "start"))
    the_game_starts_with_a_tenth_of_the_cells_filled();
  else
    return 1;
  return 0;
}
