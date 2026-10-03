#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "debris.h"
#include "game.h"
#include "game_constants.h"
#include "game_settings.h"
#include "hex_colors.h"
#include "hex_grid.h"
#include "number_text.h"
#include "playing_stage.h"
#include "random_source.h"
#include "stage.h"
#include "test_allocator.h"

// The engine's point_distance truncates to whole pixels
static double exact_distance(const point_t* a, const point_t* b) {
  return hypot(a->x - b->x, a->y - b->y);
}

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
  move_playing_stage_pointer(state, centre.x, centre.y, false);
  assert(state->has_hovered_cell);
  assert(state->hovered_cell.col == 3 && state->hovered_cell.row == 2);
  move_playing_stage_pointer(state, -500, -500, false);
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

static void new_hexagons_carry_a_power_of_two_up_to_eight(void) {
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
    bool power_of_two = (value & (value - 1)) == 0;
    if (power_of_two) {
      // 1, 2, 4 and 8, roughly evenly: 200 expected of each
      assert(seen[value] > 150 && seen[value] < 250);
    } else {
      assert(seen[value] == 0);
    }
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
  // The numbers are the powers of two from 1 to 1024
  for (int value = 1; value <= MAX_HEXAGON_VALUE; value *= 2) {
    assert(hex_border_color(value) == hex_border_color(value));
    color_t a = hex_border_color(value);
    // Bright enough to see on black
    assert(R(a) + G(a) + B(a) >= 300);
    for (int other = value * 2; other <= MAX_HEXAGON_VALUE; other *= 2) {
      color_t b = hex_border_color(other);
      // Tell them apart at a glance, not just by one shade
      int difference = abs(R(a) - R(b)) + abs(G(a) - G(b)) + abs(B(a) - B(b));
      assert(difference >= 100);
    }
    if (value < MAX_HEXAGON_VALUE) {
      // A number and its double are often side by side: far apart in hue
      color_t next = hex_border_color(value * 2);
      int difference =
          abs(R(a) - R(next)) + abs(G(a) - G(next)) + abs(B(a) - B(next));
      assert(difference >= 250);
    }
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

static bool is_selected(const board_t* board, cell_t cell) {
  return board->has_selection && board->selection.col == cell.col &&
         board->selection.row == cell.row;
}

static void clicking_a_hexagon_selects_it_and_clicking_again_unselects(void) {
  board_t board;
  assert(init_board(&board, 6, 5));
  cell_t first = {1, 1}, second = {4, 3}, empty = {2, 2};
  set_board_value(&board, first, 3);
  set_board_value(&board, second, 7);
  assert(!board.has_selection);

  click_board_cell(&board, first);
  assert(is_selected(&board, first));

  click_board_cell(&board, first);
  assert(!board.has_selection);

  // Clicking another hexagon moves the selection to it
  click_board_cell(&board, first);
  click_board_cell(&board, second);
  assert(is_selected(&board, second));
  assert(!is_selected(&board, first));

  // There is nothing to select on an empty cell or off the board
  click_board_cell(&board, second);
  click_board_cell(&board, empty);
  assert(!board.has_selection);
  cell_t outside = {6, 0};
  click_board_cell(&board, outside);
  assert(!board.has_selection);
  destroy_board(&board);
}

static cell_t first_hexagon(const board_t* board) {
  for (int row = 0; row < board->rows; ++row) {
    for (int col = 0; col < board->cols; ++col) {
      cell_t cell = {col, row};
      if (board_value(board, cell) != EMPTY_CELL) return cell;
    }
  }
  assert(false);
  cell_t none = {-1, -1};
  return none;
}

static void a_held_button_is_a_single_click(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  cell_t cell = first_hexagon(&state->board);
  point_t centre = hex_cell_centre(&state->grid, cell);

  // Pointing without pressing selects nothing
  move_playing_stage_pointer(state, centre.x, centre.y, false);
  assert(!state->board.has_selection);

  // The button stays down over several frames: still one click
  for (int frame = 0; frame < 5; ++frame) {
    move_playing_stage_pointer(state, centre.x, centre.y, true);
    assert(is_selected(&state->board, cell));
  }

  // Released and pressed again: a second click, which unselects
  move_playing_stage_pointer(state, centre.x, centre.y, false);
  assert(is_selected(&state->board, cell));
  move_playing_stage_pointer(state, centre.x, centre.y, true);
  assert(!state->board.has_selection);

  // A click outside the grid changes nothing
  move_playing_stage_pointer(state, centre.x, centre.y, false);
  move_playing_stage_pointer(state, centre.x, centre.y, true);
  move_playing_stage_pointer(state, -500, -500, false);
  move_playing_stage_pointer(state, -500, -500, true);
  assert(is_selected(&state->board, cell));
  destroy_playing_stage(state);
}

static void a_selected_hexagon_is_filled_with_a_light_tone_of_its_border(void) {
  for (int value = 1; value <= 1024; ++value) {
    color_t border = hex_border_color(value);
    color_t fill = hex_selected_fill_color(value);
    // Lighter in every channel, so the same hue washed out towards white
    assert(R(fill) >= R(border) && G(fill) >= G(border) &&
           B(fill) >= B(border));
    int lift =
        (R(fill) - R(border)) + (G(fill) - G(border)) + (B(fill) - B(border));
    assert(lift >= 100);
    // Still a tint: clearly not the black of an unselected hexagon, and
    // not plain white
    assert(fill != COLOR_WHITE);
    assert(R(fill) + G(fill) + B(fill) >= 450);
  }
}

static bool same_cell(cell_t a, cell_t b) {
  return a.col == b.col && a.row == b.row;
}

static void every_cell_has_six_neighbours_one_step_away(void) {
  for (int row = 2; row < 6; ++row) {
    for (int col = 2; col < 6; ++col) {
      cell_t cell = {col, row};
      assert(hex_distance(cell, cell) == 0);
      for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
        cell_t neighbour = hex_neighbour(cell, direction);
        assert(hex_distance(cell, neighbour) == 1);
        // Going back the opposite way returns to the cell
        int opposite = (direction + 3) % HEX_DIRECTION_COUNT;
        assert(same_cell(hex_neighbour(neighbour, opposite), cell));
        // Keeping the same direction goes in a straight line
        assert(hex_distance(cell, hex_neighbour(neighbour, direction)) == 2);
        for (int other = direction + 1; other < HEX_DIRECTION_COUNT; ++other) {
          assert(!same_cell(neighbour, hex_neighbour(cell, other)));
        }
      }
    }
  }
  // Neighbours on screen are one hexagon width apart
  hex_grid_t grid = create_hex_grid(1440, 900, 37.5);
  cell_t cell = {5, 5};
  point_t centre = hex_cell_centre(&grid, cell);
  for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
    point_t other = hex_cell_centre(&grid, hex_neighbour(cell, direction));
    assert(fabs(exact_distance(&centre, &other) - sqrt(3) * 37.5) < 1e-6);
  }
}

static void assert_path_is_connected(const board_t* board, cell_t from,
                                     cell_t to) {
  assert(board->path_length >= 2);
  assert(same_cell(board->path[0], from));
  assert(same_cell(board->path[board->path_length - 1], to));
  for (int i = 1; i < board->path_length; ++i) {
    assert(hex_distance(board->path[i - 1], board->path[i]) == 1);
  }
}

static void on_an_empty_board_the_path_is_as_long_as_the_distance(void) {
  board_t board;
  assert(init_board(&board, 9, 8));
  for (int from_index = 0; from_index < 72; from_index += 5) {
    for (int to_index = 0; to_index < 72; ++to_index) {
      if (from_index == to_index) continue;
      cell_t from = {from_index % 9, from_index / 9};
      cell_t to = {to_index % 9, to_index / 9};
      set_board_value(&board, from, 4);
      assert(click_board_cell(&board, from) == CLICK_SELECTED);
      assert(click_board_cell(&board, to) == CLICK_MOVED);
      assert_path_is_connected(&board, from, to);
      assert(board.path_length - 1 == hex_distance(from, to));
      // The hexagon is now on the target and nothing is selected
      assert(board_value(&board, from) == EMPTY_CELL);
      assert(board_value(&board, to) == 4);
      assert(!board.has_selection);
      assert(board_hexagon_count(&board) == 1);
      set_board_value(&board, to, EMPTY_CELL);
    }
  }
  destroy_board(&board);
}

static void the_path_goes_around_other_hexagons(void) {
  board_t board;
  assert(init_board(&board, 7, 7));
  // A wall across row 3 with a single gap at the far right
  for (int col = 0; col < 6; ++col) {
    cell_t cell = {col, 3};
    set_board_value(&board, cell, 2);
  }
  cell_t from = {0, 1}, to = {0, 5};
  set_board_value(&board, from, 5);
  click_board_cell(&board, from);
  assert(click_board_cell(&board, to) == CLICK_MOVED);
  assert_path_is_connected(&board, from, to);
  assert(board.path_length - 1 > hex_distance(from, to));
  bool through_gap = false;
  for (int i = 1; i < board.path_length - 1; ++i) {
    // Every cell on the way was free
    assert(board_value(&board, board.path[i]) == EMPTY_CELL);
    cell_t gap = {6, 3};
    through_gap = through_gap || same_cell(board.path[i], gap);
  }
  assert(through_gap);
  destroy_board(&board);
}

static void an_enclosed_hexagon_cannot_move(void) {
  board_t board;
  assert(init_board(&board, 7, 7));
  cell_t enclosed = {3, 3}, target = {0, 0};
  set_board_value(&board, enclosed, 6);
  for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
    set_board_value(&board, hex_neighbour(enclosed, direction), 1);
  }
  click_board_cell(&board, enclosed);
  assert(click_board_cell(&board, target) == CLICK_IGNORED);
  // Nothing moved and the selection stays
  assert(board_value(&board, enclosed) == 6);
  assert(board_value(&board, target) == EMPTY_CELL);
  assert(board.has_selection && same_cell(board.selection, enclosed));
  assert(board_hexagon_count(&board) == 7);

  // Unreachable from the outside too: the centre of a closed ring
  set_board_value(&board, enclosed, EMPTY_CELL);
  board.has_selection = false;
  cell_t outsider = {0, 6};
  set_board_value(&board, outsider, 8);
  click_board_cell(&board, outsider);
  assert(click_board_cell(&board, enclosed) == CLICK_IGNORED);
  assert(board_value(&board, outsider) == 8);
  destroy_board(&board);
}

static void a_board_that_cannot_be_allocated_leaves_nothing_behind(void) {
  bool succeeded = false;
  for (int allocation = 0; allocation < 16 && !succeeded; ++allocation) {
    fail_allocation_after(allocation);
    board_t board;
    succeeded = init_board(&board, 5, 4);
    if (succeeded) destroy_board(&board);
    assert(outstanding_allocations() == 0);
  }
  assert(succeeded);
  fail_allocation_after(-1);
}

// An empty cell far enough from the hexagon to make a journey of it
static cell_t distant_empty_cell(const board_t* board, cell_t from) {
  for (int row = board->rows - 1; row >= 0; --row) {
    for (int col = board->cols - 1; col >= 0; --col) {
      cell_t cell = {col, row};
      if (board_value(board, cell) == EMPTY_CELL &&
          hex_distance(from, cell) >= 5)
        return cell;
    }
  }
  assert(false);
  return from;
}

static void click_at(playing_stage_state_ptr state, cell_t cell) {
  point_t centre = hex_cell_centre(&state->grid, cell);
  move_playing_stage_pointer(state, centre.x, centre.y, false);
  move_playing_stage_pointer(state, centre.x, centre.y, true);
  move_playing_stage_pointer(state, centre.x, centre.y, false);
}

static void a_moved_hexagon_travels_along_its_path(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  cell_t from = first_hexagon(&state->board);
  cell_t to = distant_empty_cell(&state->board, from);
  int value = board_value(&state->board, from);
  assert(!is_playing_stage_travelling(state));

  click_at(state, from);
  click_at(state, to);
  assert(board_value(&state->board, to) == value);
  assert(is_playing_stage_travelling(state));

  // It sets off from where it was
  point_t start = hex_cell_centre(&state->grid, from);
  point_t end = hex_cell_centre(&state->grid, to);
  point_t position = travelling_hexagon_position(state);
  assert(exact_distance(&position, &start) < 1e-6);

  // Clicks wait until it has arrived
  click_at(state, to);
  assert(!state->board.has_selection);

  // It never jumps: each frame moves it less than one cell
  int steps = state->board.path_length - 1;
  double step_length = sqrt(3) * state->grid.radius;
  int frames = 0;
  while (is_playing_stage_travelling(state)) {
    point_t before = travelling_hexagon_position(state);
    advance_playing_stage(state, 1.0);
    point_t after = travelling_hexagon_position(state);
    assert(exact_distance(&before, &after) < step_length);
    assert(++frames < 10000);
  }
  position = travelling_hexagon_position(state);
  assert(exact_distance(&position, &end) < 1e-6);
  // The journey takes time in proportion to its length
  double expected_frames = steps * 60.0 / TRAVEL_STEPS_PER_SECOND;
  assert(fabs(frames - expected_frames) <= 1);

  // Once it has arrived it can be picked up again
  click_at(state, to);
  assert(state->board.has_selection);
  destroy_playing_stage(state);
}

static void a_stalled_frame_cannot_break_the_journey(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  cell_t from = first_hexagon(&state->board);
  cell_t to = distant_empty_cell(&state->board, from);
  click_at(state, from);
  click_at(state, to);
  point_t start = hex_cell_centre(&state->grid, from);
  advance_playing_stage(state, NAN);
  advance_playing_stage(state, -5);
  advance_playing_stage(state, 0);
  point_t position = travelling_hexagon_position(state);
  assert(exact_distance(&position, &start) < 1e-6);
  advance_playing_stage(state, 1e12);
  assert(!is_playing_stage_travelling(state));
  point_t end = hex_cell_centre(&state->grid, to);
  position = travelling_hexagon_position(state);
  assert(exact_distance(&position, &end) < 1e-6);
  destroy_playing_stage(state);
}

static void numbers_are_sized_to_fit_inside_their_hexagon(void) {
  const int heights[] = {600, 900, 1080, 1964};
  for (int i = 0; i < 4; ++i) {
    double width = heights[i] / 20.0, height = heights[i] / 40.0;
    int previous = 1 << 30;
    for (int digits = 1; digits <= MAX_NUMBER_DIGITS; ++digits) {
      int size = number_font_size(digits, width, height);
      // The glyphs of this font are as wide as they are tall
      assert(size >= 1);
      assert(size * digits <= width && size <= height);
      // A pixel font is only crisp at multiples of its 8 pixel grid
      int step = size >= 8 ? 8 : 1;
      assert(size % step == 0);
      // No room for the next size up
      assert((size + step) * digits > width || size + step > height);
      // Longer numbers are never written bigger than shorter ones
      assert(size <= previous);
      previous = size;
    }
  }
  // Even an absurdly small box gets a size that can be loaded
  assert(number_font_size(4, 1, 1) == 1);
}

static void the_number_font_loads_and_its_numbers_fit(void) {
  assert(init_ttf_system());
  double width = 40, height = 22;
  number_text_t numbers = load_number_text(width, height);
  const int values[] = {1, 8, 35, 99, 128, 512, 999, 1024};
  for (int i = 0; i < 8; ++i) {
    int text_width = 0, text_height = 0;
    assert(measure_number_text(&numbers, values[i], &text_width, &text_height));
    assert(text_width > 0 && text_width <= width);
    assert(text_height > 0 && text_height <= height);
  }
  // Values no hexagon can carry have no text
  int unused;
  assert(!measure_number_text(&numbers, 0, &unused, &unused));
  assert(
      !measure_number_text(&numbers, MAX_HEXAGON_VALUE + 1, &unused, &unused));
  free_number_text(&numbers);

  // Without the font there is nothing to measure, and nothing breaks
  number_text_t missing = {0};
  assert(!measure_number_text(&missing, 5, &unused, &unused));
  free_number_text(&missing);
  quit_ttf_system();
}

// Put hexagons of one value on consecutive cells in a straight line
static cell_t lay_line(board_t* board, cell_t start, int direction, int count,
                       int value) {
  cell_t cell = start;
  for (int i = 0; i < count; ++i) {
    set_board_value(board, cell, value);
    cell = hex_neighbour(cell, direction);
  }
  return cell;  // The cell just past the end of the line
}

// The player moves a new hexagon of the given value onto a cell
static bool play_hexagon_onto(board_t* board, cell_t target, int value) {
  cell_t from = {0, 0};
  assert(board_value(board, from) == EMPTY_CELL);
  set_board_value(board, from, value);
  assert(click_board_cell(board, from) == CLICK_SELECTED);
  assert(click_board_cell(board, target) == CLICK_MOVED);
  // These boards are laid out by hand: keep new hexagons off them
  board->spawn_count = 0;
  random_source_t random = create_random_source(1);
  settle_result_t result = settle_board_move(board, &random);
  return result == SETTLED_MERGE || result == SETTLED_WALL;
}

static void a_line_merges_into_four_times_its_number_where_the_move_ended(
    void) {
  for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
    for (int length = 4; length <= 6; ++length) {
      // The moved hexagon completes the line at any position along it
      for (int gap = 0; gap < length; ++gap) {
        board_t board;
        assert(init_board(&board, 16, 16));
        cell_t start = {8, 8};
        lay_line(&board, start, direction, length, 2);
        cell_t target = start;
        for (int i = 0; i < gap; ++i) target = hex_neighbour(target, direction);
        set_board_value(&board, target, EMPTY_CELL);
        assert(board_hexagon_count(&board) == length - 1);

        assert(play_hexagon_onto(&board, target, 2));
        assert(board_hexagon_count(&board) == 1);
        // Four times the number however long the line, so that every
        // number stays a power of two
        assert(board_value(&board, target) == 8);
        destroy_board(&board);
      }
    }
  }
}

static void three_in_a_line_do_not_merge(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  cell_t start = {8, 8};
  cell_t target = lay_line(&board, start, 0, 2, 5);
  assert(!play_hexagon_onto(&board, target, 5));
  assert(board_hexagon_count(&board) == 3);
  assert(board_value(&board, target) == 5);
  destroy_board(&board);
}

static void only_equal_numbers_make_a_line(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  // 2 2 3 then the moved 2: four in a line, but not four equal
  cell_t start = {4, 8};
  cell_t third = lay_line(&board, start, 0, 2, 2);
  cell_t target = lay_line(&board, third, 0, 1, 3);
  assert(!play_hexagon_onto(&board, target, 2));
  assert(board_hexagon_count(&board) == 4);

  // 2 2 2 3 2 then the moved 2 next to the last: the 3 breaks the run
  board_t broken;
  assert(init_board(&broken, 16, 16));
  cell_t breaker = lay_line(&broken, start, 0, 3, 2);
  cell_t after = lay_line(&broken, breaker, 0, 1, 3);
  cell_t end = lay_line(&broken, after, 0, 1, 2);
  assert(!play_hexagon_onto(&broken, end, 2));
  assert(board_hexagon_count(&broken) == 6);
  destroy_board(&board);
  destroy_board(&broken);
}

static void lines_crossing_where_the_move_ended_merge_together(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  cell_t target = {8, 8};
  // Three to the east and three to the south-east of the target
  lay_line(&board, hex_neighbour(target, 0), 0, 3, 4);
  lay_line(&board, hex_neighbour(target, 1), 1, 3, 4);
  // And one lone 4 to the south-west: a line of two, which stays
  cell_t bystander = hex_neighbour(target, 2);
  set_board_value(&board, bystander, 4);

  assert(play_hexagon_onto(&board, target, 4));
  // Seven hexagons merged into one, still worth four times the number
  assert(board_value(&board, target) == 16);
  assert(board_value(&board, bystander) == 4);
  assert(board_hexagon_count(&board) == 2);
  destroy_board(&board);
}

static void a_line_merges_only_when_a_move_completes_it(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  // A line of four that was already there, and a move elsewhere
  cell_t start = {4, 12};
  lay_line(&board, start, 0, 4, 6);
  cell_t target = {8, 3};
  assert(!play_hexagon_onto(&board, target, 6));
  assert(board_hexagon_count(&board) == 5);

  // The merge waits for the move to be settled, and happens once
  cell_t line = {4, 6};
  cell_t end = lay_line(&board, line, 0, 3, 1);
  cell_t from = {0, 0};
  set_board_value(&board, from, 1);
  click_board_cell(&board, from);
  assert(click_board_cell(&board, end) == CLICK_MOVED);
  assert(board_value(&board, end) == 1);
  assert(board_hexagon_count(&board) == 9);
  random_source_t random = create_random_source(1);
  assert(settle_board_move(&board, &random) == SETTLED_MERGE);
  assert(board_value(&board, end) == 4);
  assert(board_hexagon_count(&board) == 6);
  assert(settle_board_move(&board, &random) == SETTLED_NOTHING);
  assert(board_hexagon_count(&board) == 6);
  assert(board_value(&board, end) == 4);
  destroy_board(&board);
}

static void the_merge_shows_when_the_hexagon_arrives(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  board_t* board = &state->board;
  for (int row = 0; row < board->rows; ++row) {
    for (int col = 0; col < board->cols; ++col) {
      cell_t cell = {col, row};
      set_board_value(board, cell, EMPTY_CELL);
    }
  }
  cell_t start = {5, 5}, from = {12, 10};
  cell_t target = lay_line(board, start, 0, 3, 3);
  set_board_value(board, from, 3);
  click_at(state, from);
  click_at(state, target);
  assert(is_playing_stage_travelling(state));
  // Still on its way: nothing has merged yet
  assert(board_hexagon_count(board) == 4);
  while (is_playing_stage_travelling(state)) {
    assert(board_value(board, target) == 3);
    advance_playing_stage(state, 1.0);
  }
  assert(board_value(board, target) == 12);
  assert(board_hexagon_count(board) == 1);
  destroy_playing_stage(state);
}

static void a_hexagon_bursts_into_its_six_wedges(void) {
  debris_t debris;
  assert(init_debris(&debris, 60));
  assert(debris_piece_count(&debris) == 0);
  random_source_t random = create_random_source(11);
  point_t centre = point(400, 300);
  burst_hexagon(&debris, &random, centre, 30, 0x12AB34);
  assert(debris_piece_count(&debris) == DEBRIS_PIECES_PER_HEXAGON);
  // Before they move, the pieces are the hexagon cut into wedges: each one
  // spans the centre and two neighbouring corners
  for (int i = 0; i < DEBRIS_PIECES_PER_HEXAGON; ++i) {
    const debris_piece_t* piece = &debris.pieces[i];
    assert(piece->active && piece->color == 0x12AB34);
    point_t corners[DEBRIS_PIECE_CORNERS];
    debris_piece_corners(piece, corners);
    point_t expected[DEBRIS_PIECE_CORNERS] = {
        centre, hex_corner(centre, 30, i),
        hex_corner(centre, 30, (i + 1) % HEX_CORNER_COUNT)};
    for (int corner = 0; corner < DEBRIS_PIECE_CORNERS; ++corner) {
      assert(exact_distance(&corners[corner], &expected[corner]) < 1e-6);
    }
  }
  destroy_debris(&debris);
  assert(outstanding_allocations() == 0);
}

static void debris_flies_up_then_falls_off_the_screen(void) {
  debris_t debris;
  assert(init_debris(&debris, 60));
  random_source_t random = create_random_source(11);
  double radius = 30, floor = 900;
  point_t centre = point(400, 450);
  burst_hexagon(&debris, &random, centre, radius, COLOR_RED);
  double start[DEBRIS_PIECES_PER_HEXAGON], highest[DEBRIS_PIECES_PER_HEXAGON];
  for (int i = 0; i < DEBRIS_PIECES_PER_HEXAGON; ++i) {
    start[i] = highest[i] = debris.pieces[i].position.y;
  }
  // Time that makes no sense changes nothing
  advance_debris(&debris, NAN, floor);
  advance_debris(&debris, -3, floor);
  advance_debris(&debris, 0, floor);
  for (int i = 0; i < DEBRIS_PIECES_PER_HEXAGON; ++i) {
    assert(debris.pieces[i].position.y == start[i]);
  }

  int frames = 0;
  while (debris_piece_count(&debris) > 0) {
    advance_debris(&debris, 1.0, floor);
    ++frames;
    for (int i = 0; i < DEBRIS_PIECES_PER_HEXAGON; ++i) {
      const debris_piece_t* piece = &debris.pieces[i];
      if (frames == 1) {
        // Every piece sets off upwards: y shrinks towards the top
        assert(piece->position.y < start[i]);
      }
      if (piece->active) {
        highest[i] = fmin(highest[i], piece->position.y);
      } else {
        // A piece only goes once it is wholly below the screen
        assert(piece->position.y - radius > floor);
      }
    }
    // Over in a few seconds
    assert(frames < 300);
  }
  for (int i = 0; i < DEBRIS_PIECES_PER_HEXAGON; ++i) {
    // Each rose well clear of where the hexagon was
    assert(start[i] - highest[i] > 2 * radius);
  }
  // And it lasts long enough to be seen
  assert(frames > 30);
  destroy_debris(&debris);
}

static void debris_beyond_capacity_is_dropped(void) {
  debris_t debris;
  assert(init_debris(&debris, 10));
  random_source_t random = create_random_source(11);
  burst_hexagon(&debris, &random, point(100, 100), 30, COLOR_RED);
  burst_hexagon(&debris, &random, point(200, 100), 30, COLOR_RED);
  burst_hexagon(&debris, &random, point(300, 100), 30, COLOR_RED);
  assert(debris_piece_count(&debris) == 10);
  // Once the pieces have gone there is room again
  advance_debris(&debris, 1e9, 900);
  assert(debris_piece_count(&debris) == 0);
  burst_hexagon(&debris, &random, point(100, 100), 30, COLOR_RED);
  assert(debris_piece_count(&debris) == DEBRIS_PIECES_PER_HEXAGON);
  destroy_debris(&debris);

  fail_allocation_after(0);
  assert(!init_debris(&debris, 10));
  fail_allocation_after(-1);
  destroy_debris(&debris);
  assert(outstanding_allocations() == 0);
}

static void merged_hexagons_explode(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  board_t* board = &state->board;
  for (int row = 0; row < board->rows; ++row) {
    for (int col = 0; col < board->cols; ++col) {
      cell_t cell = {col, row};
      set_board_value(board, cell, EMPTY_CELL);
    }
  }
  cell_t start = {5, 5}, from = {12, 10};
  cell_t target = lay_line(board, start, 0, 3, 2);
  set_board_value(board, from, 2);
  click_at(state, from);
  click_at(state, target);
  while (is_playing_stage_travelling(state)) {
    assert(debris_piece_count(&state->debris) == 0);
    advance_playing_stage(state, 1.0);
  }
  // The three hexagons that vanished burst; the one that stays does not
  assert(debris_piece_count(&state->debris) == 3 * DEBRIS_PIECES_PER_HEXAGON);
  point_t target_centre = hex_cell_centre(&state->grid, target);
  for (int i = 0; i < state->debris.capacity; ++i) {
    const debris_piece_t* piece = &state->debris.pieces[i];
    if (!piece->active) continue;
    // In the colour the hexagons had, not that of their sum
    assert(piece->color == hex_border_color(2));
    assert(piece->position.y <= target_centre.y + state->grid.radius);
    assert(piece->position.x < target_centre.x);
  }
  // The pieces keep flying while the player gets on with the game
  click_at(state, target);
  assert(state->board.has_selection);
  for (int frame = 0; frame < 300; ++frame) advance_playing_stage(state, 1.0);
  assert(debris_piece_count(&state->debris) == 0);

  // A move that merges nothing bursts nothing
  cell_t elsewhere = {2, 12};
  click_at(state, elsewhere);
  while (is_playing_stage_travelling(state)) advance_playing_stage(state, 1.0);
  assert(debris_piece_count(&state->debris) == 0);
  destroy_playing_stage(state);
  assert(outstanding_allocations() == 0);
}

static void a_move_that_merges_nothing_brings_new_hexagons(void) {
  const int sizes[][3] = {{21, 15, 3}, {16, 16, 3}, {5, 5, 1}, {30, 20, 6}};
  for (int i = 0; i < 4; ++i) {
    int cells = sizes[i][0] * sizes[i][1];
    assert(spawn_hexagon_count(cells) == sizes[i][2]);
    board_t board;
    assert(init_board(&board, sizes[i][0], sizes[i][1]));
    random_source_t random = create_random_source(9);
    cell_t from = {0, 0}, to = {3, 3};
    set_board_value(&board, from, 16);
    click_board_cell(&board, from);
    assert(click_board_cell(&board, to) == CLICK_MOVED);
    // Nothing new until the move is settled
    assert(board_hexagon_count(&board) == 1);
    assert(settle_board_move(&board, &random) == SETTLED_SPAWN);
    assert(board_hexagon_count(&board) == 1 + sizes[i][2]);
    assert(board_value(&board, to) == 16);
    // And only once
    assert(settle_board_move(&board, &random) == SETTLED_NOTHING);
    assert(board_hexagon_count(&board) == 1 + sizes[i][2]);
    destroy_board(&board);
  }
}

static void a_move_that_merges_brings_no_new_hexagons(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  random_source_t random = create_random_source(9);
  cell_t start = {8, 8}, from = {0, 0};
  cell_t target = lay_line(&board, start, 0, 3, 2);
  set_board_value(&board, from, 2);
  click_board_cell(&board, from);
  assert(click_board_cell(&board, target) == CLICK_MOVED);
  assert(settle_board_move(&board, &random) == SETTLED_MERGE);
  assert(board_hexagon_count(&board) == 1);
  assert(board_value(&board, target) == 8);
  destroy_board(&board);
}

static void new_hexagons_fill_what_room_is_left(void) {
  board_t board;
  assert(init_board(&board, 30, 20));
  assert(board.spawn_count == 6);
  random_source_t random = create_random_source(9);
  // Fill the board with numbers that never line up four equal, but for
  // three empty cells and the hexagon about to move
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 30; ++col) {
      cell_t cell = {col, row};
      set_board_value(&board, cell, 16 << ((col + row * 2) % 3));
    }
  }
  cell_t from = {0, 0}, to = {1, 0}, spare = {5, 5}, other = {20, 12};
  set_board_value(&board, to, EMPTY_CELL);
  set_board_value(&board, spare, EMPTY_CELL);
  set_board_value(&board, other, EMPTY_CELL);
  set_board_value(&board, from, 1024);
  click_board_cell(&board, from);
  assert(click_board_cell(&board, to) == CLICK_MOVED);
  assert(settle_board_move(&board, &random) == SETTLED_SPAWN);
  // Three cells were free: the one it left and the two spares
  assert(board_hexagon_count(&board) == 600);
  destroy_board(&board);
}

static void new_hexagons_never_merge_by_themselves(void) {
  // Rows of equal numbers with a gap every fifth cell: one new hexagon in
  // four lands on a gap that completes a line
  for (uint32_t seed = 0; seed < 200; ++seed) {
    board_t board;
    assert(init_board(&board, 20, 10));
    for (int row = 2; row < 10; ++row) {
      for (int col = 0; col < 20; ++col) {
        cell_t cell = {col, row};
        if (col % 5 != 4) set_board_value(&board, cell, 1 << (row % 4));
      }
    }
    int before = board_hexagon_count(&board);
    random_source_t random = create_random_source(seed);
    cell_t from = {0, 0}, to = {10, 0};
    set_board_value(&board, from, 16);
    click_board_cell(&board, from);
    assert(click_board_cell(&board, to) == CLICK_MOVED);
    assert(settle_board_move(&board, &random) == SETTLED_SPAWN);
    assert(board.merged_count == 0);
    assert(board_hexagon_count(&board) == before + 1 + board.spawn_count);
    destroy_board(&board);
  }
}

static void new_hexagons_appear_when_the_moved_one_arrives(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  int before = board_hexagon_count(&state->board);
  cell_t from = first_hexagon(&state->board);
  // Wherever it goes, park it clear of any line it could complete
  set_board_value(&state->board, from, 1024);
  cell_t to = distant_empty_cell(&state->board, from);
  click_at(state, from);
  click_at(state, to);
  while (is_playing_stage_travelling(state)) {
    assert(board_hexagon_count(&state->board) == before);
    advance_playing_stage(state, 1.0);
  }
  int cells = hex_grid_cell_count(&state->grid);
  assert(board_hexagon_count(&state->board) ==
         before + spawn_hexagon_count(cells));
  destroy_playing_stage(state);
}

static void a_merge_beyond_the_highest_number_turns_the_line_to_wall(void) {
  const int values[] = {512, 1024};
  for (int i = 0; i < 2; ++i) {
    board_t board;
    assert(init_board(&board, 16, 16));
    cell_t start = {8, 8};
    cell_t target = lay_line(&board, start, 0, 3, values[i]);
    cell_t bystander = {3, 12};
    set_board_value(&board, bystander, values[i]);
    assert(play_hexagon_onto(&board, target, values[i]));
    // Four times the number would pass 1024: every hexagon in the line,
    // the moved one included, is wall instead
    cell_t cell = start;
    for (int n = 0; n < 4; ++n) {
      assert(board_value(&board, cell) == WALL_CELL);
      cell = hex_neighbour(cell, 0);
    }
    assert(board_value(&board, bystander) == values[i]);
    assert(board_wall_count(&board) == 4);
    // Walls are not hexagons
    assert(board_hexagon_count(&board) == 1);
    destroy_board(&board);
  }

  // 256 is the last number that can still merge: it makes 1024
  board_t board;
  assert(init_board(&board, 16, 16));
  cell_t start = {8, 8};
  cell_t target = lay_line(&board, start, 0, 3, 256);
  assert(play_hexagon_onto(&board, target, 256));
  assert(board_value(&board, target) == MAX_HEXAGON_VALUE);
  assert(board_wall_count(&board) == 0);
  destroy_board(&board);
}

static void walls_cannot_be_selected_moved_or_crossed(void) {
  board_t board;
  assert(init_board(&board, 7, 7));
  // A wall across the whole of row 3
  for (int col = 0; col < 7; ++col) {
    cell_t cell = {col, 3};
    set_board_value(&board, cell, WALL_CELL);
  }
  cell_t wall = {2, 3}, hexagon = {1, 1}, same_side = {5, 2}, far_side = {1, 5};
  set_board_value(&board, hexagon, 4);

  assert(click_board_cell(&board, wall) == CLICK_IGNORED);
  assert(!board.has_selection);

  click_board_cell(&board, hexagon);
  // Clicking a wall with a hexagon selected changes nothing
  assert(click_board_cell(&board, wall) == CLICK_IGNORED);
  assert(board.has_selection && same_cell(board.selection, hexagon));
  assert(board_value(&board, wall) == WALL_CELL);
  // There is no way through to the other side
  assert(click_board_cell(&board, far_side) == CLICK_IGNORED);
  assert(board_value(&board, hexagon) == 4);
  // But the hexagon is free on its own side
  assert(click_board_cell(&board, same_side) == CLICK_MOVED);
  for (int i = 0; i < board.path_length; ++i) assert(board.path[i].row < 3);
  destroy_board(&board);
}

static void walls_take_no_part_in_lines_or_new_hexagons(void) {
  board_t board;
  assert(init_board(&board, 16, 16));
  // Three walls in a row are not three equal numbers
  cell_t start = {8, 8};
  cell_t target = lay_line(&board, start, 0, 3, WALL_CELL);
  assert(!play_hexagon_onto(&board, target, 2));
  assert(board_wall_count(&board) == 3);
  destroy_board(&board);

  // New hexagons never land on a wall
  assert(init_board(&board, 4, 3));
  cell_t wall = {1, 1};
  set_board_value(&board, wall, WALL_CELL);
  random_source_t random = create_random_source(5);
  assert(populate_board(&board, &random, 50) == 11);
  assert(board_value(&board, wall) == WALL_CELL);
  assert(board_hexagon_count(&board) == 11);
  destroy_board(&board);
}

static void a_line_turned_to_wall_does_not_explode(void) {
  game_t game = test_game();
  playing_stage_state_ptr state = create_playing_stage(&game);
  board_t* board = &state->board;
  for (int row = 0; row < board->rows; ++row) {
    for (int col = 0; col < board->cols; ++col) {
      cell_t cell = {col, row};
      set_board_value(board, cell, EMPTY_CELL);
    }
  }
  cell_t start = {5, 5}, from = {12, 10};
  cell_t target = lay_line(board, start, 0, 3, 1024);
  set_board_value(board, from, 1024);
  click_at(state, from);
  click_at(state, target);
  while (is_playing_stage_travelling(state)) advance_playing_stage(state, 1.0);
  assert(board_wall_count(board) == 4);
  assert(debris_piece_count(&state->debris) == 0);
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
  else if (!strcmp(argv[1], "population"))
    a_new_board_holds_a_tenth_of_its_cells();
  else if (!strcmp(argv[1], "values"))
    new_hexagons_carry_a_power_of_two_up_to_eight();
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
  else if (!strcmp(argv[1], "selection"))
    clicking_a_hexagon_selects_it_and_clicking_again_unselects();
  else if (!strcmp(argv[1], "click"))
    a_held_button_is_a_single_click();
  else if (!strcmp(argv[1], "tone"))
    a_selected_hexagon_is_filled_with_a_light_tone_of_its_border();
  else if (!strcmp(argv[1], "neighbours"))
    every_cell_has_six_neighbours_one_step_away();
  else if (!strcmp(argv[1], "path"))
    on_an_empty_board_the_path_is_as_long_as_the_distance();
  else if (!strcmp(argv[1], "detour"))
    the_path_goes_around_other_hexagons();
  else if (!strcmp(argv[1], "enclosed"))
    an_enclosed_hexagon_cannot_move();
  else if (!strcmp(argv[1], "board_allocation"))
    a_board_that_cannot_be_allocated_leaves_nothing_behind();
  else if (!strcmp(argv[1], "travel"))
    a_moved_hexagon_travels_along_its_path();
  else if (!strcmp(argv[1], "stall"))
    a_stalled_frame_cannot_break_the_journey();
  else if (!strcmp(argv[1], "font_size"))
    numbers_are_sized_to_fit_inside_their_hexagon();
  else if (!strcmp(argv[1], "font"))
    the_number_font_loads_and_its_numbers_fit();
  else if (!strcmp(argv[1], "merge"))
    a_line_merges_into_four_times_its_number_where_the_move_ended();
  else if (!strcmp(argv[1], "three"))
    three_in_a_line_do_not_merge();
  else if (!strcmp(argv[1], "equal"))
    only_equal_numbers_make_a_line();
  else if (!strcmp(argv[1], "crossing"))
    lines_crossing_where_the_move_ended_merge_together();
  else if (!strcmp(argv[1], "settle"))
    a_line_merges_only_when_a_move_completes_it();
  else if (!strcmp(argv[1], "arrival"))
    the_merge_shows_when_the_hexagon_arrives();
  else if (!strcmp(argv[1], "burst"))
    a_hexagon_bursts_into_its_six_wedges();
  else if (!strcmp(argv[1], "arc"))
    debris_flies_up_then_falls_off_the_screen();
  else if (!strcmp(argv[1], "debris_capacity"))
    debris_beyond_capacity_is_dropped();
  else if (!strcmp(argv[1], "explosion"))
    merged_hexagons_explode();
  else if (!strcmp(argv[1], "spawn"))
    a_move_that_merges_nothing_brings_new_hexagons();
  else if (!strcmp(argv[1], "no_spawn"))
    a_move_that_merges_brings_no_new_hexagons();
  else if (!strcmp(argv[1], "spawn_room"))
    new_hexagons_fill_what_room_is_left();
  else if (!strcmp(argv[1], "spawn_merge"))
    new_hexagons_never_merge_by_themselves();
  else if (!strcmp(argv[1], "spawn_arrival"))
    new_hexagons_appear_when_the_moved_one_arrives();
  else if (!strcmp(argv[1], "wall"))
    a_merge_beyond_the_highest_number_turns_the_line_to_wall();
  else if (!strcmp(argv[1], "wall_blocks"))
    walls_cannot_be_selected_moved_or_crossed();
  else if (!strcmp(argv[1], "wall_inert"))
    walls_take_no_part_in_lines_or_new_hexagons();
  else if (!strcmp(argv[1], "wall_quiet"))
    a_line_turned_to_wall_does_not_explode();
  else
    return 1;
  return 0;
}
