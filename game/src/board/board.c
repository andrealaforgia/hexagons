#include "board.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hex_grid.h"
#include "random_source.h"

#define INITIAL_FILL_FRACTION 0.10
#define SPAWN_FRACTION 0.01
// The board on which MIN_GROUP_SIZE hexagons make a group
#define ORDINARY_CELL_COUNT 300.0
// New hexagons carry 1, 2, 4 or 8
#define NEW_HEXAGON_VALUE_COUNT 4

static int cell_count(const board_t* board) {
  return board->cols * board->rows;
}

static bool contains(const board_t* board, cell_t cell) {
  return cell.col >= 0 && cell.col < board->cols && cell.row >= 0 &&
         cell.row < board->rows;
}

static int cell_index(const board_t* board, cell_t cell) {
  return cell.row * board->cols + cell.col;
}

bool init_board(board_t* board, int cols, int rows) {
  size_t cells = (size_t)(cols * rows);
  board->cols = cols;
  board->rows = rows;
  board->has_selection = false;
  board->path_length = 0;
  board->move_pending = false;
  board->spawn_count = spawn_hexagon_count(cols * rows);
  board->min_group_size = min_group_size_for(cols * rows);
  board->merged_count = 0;
  board->merged_value = EMPTY_CELL;
  board->merged = calloc(cells, sizeof(cell_t));
  board->values = calloc(cells, sizeof(int));
  board->path = calloc(cells, sizeof(cell_t));
  board->came_from = calloc(cells, sizeof(int));
  board->queue = calloc(cells, sizeof(int));
  if (!board->values || !board->path || !board->merged || !board->came_from ||
      !board->queue) {
    destroy_board(board);
    return false;
  }
  return true;
}

void destroy_board(board_t* board) {
  free(board->values);
  free(board->path);
  free(board->merged);
  free(board->came_from);
  free(board->queue);
  board->values = NULL;
  board->path = NULL;
  board->merged = NULL;
  board->came_from = NULL;
  board->queue = NULL;
}

int board_value(const board_t* board, cell_t cell) {
  if (!contains(board, cell)) {
    return EMPTY_CELL;
  }
  return board->values[cell_index(board, cell)];
}

void set_board_value(board_t* board, cell_t cell, int value) {
  if (contains(board, cell)) {
    board->values[cell_index(board, cell)] = value;
  }
}

static int count_cells(const board_t* board, bool (*matches)(int value)) {
  int count = 0;
  for (int i = 0; i < cell_count(board); ++i) {
    if (matches(board->values[i])) {
      ++count;
    }
  }
  return count;
}

static bool is_hexagon(int value) { return value > 0; }
static bool is_wall(int value) { return value == WALL_CELL; }
static bool is_empty(int value) { return value == EMPTY_CELL; }

int board_hexagon_count(const board_t* board) {
  return count_cells(board, is_hexagon);
}

int board_wall_count(const board_t* board) {
  return count_cells(board, is_wall);
}

static bool is_selected(const board_t* board, cell_t cell) {
  return board->has_selection && board->selection.col == cell.col &&
         board->selection.row == cell.row;
}

#define UNVISITED (-1)

// Breadth-first search through empty cells, so the first way found to a
// cell is a shortest one. Fills path and returns true if there is a way.
static bool find_path(board_t* board, cell_t from, cell_t to) {
  for (int i = 0; i < cell_count(board); ++i) {
    board->came_from[i] = UNVISITED;
  }
  int start = cell_index(board, from), goal = cell_index(board, to);
  int head = 0, tail = 0;
  board->queue[tail++] = start;
  board->came_from[start] = start;
  while (head < tail && board->came_from[goal] == UNVISITED) {
    int index = board->queue[head++];
    cell_t cell = {index % board->cols, index / board->cols};
    for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
      cell_t next = hex_neighbour(cell, direction);
      if (!contains(board, next) || board_value(board, next) != EMPTY_CELL) {
        continue;
      }
      int next_index = cell_index(board, next);
      if (board->came_from[next_index] == UNVISITED) {
        board->came_from[next_index] = index;
        board->queue[tail++] = next_index;
      }
    }
  }
  if (board->came_from[goal] == UNVISITED) {
    return false;
  }
  int length = 1;
  for (int index = goal; index != start; index = board->came_from[index]) {
    ++length;
  }
  board->path_length = length;
  for (int index = goal, i = length - 1; i >= 0;
       index = board->came_from[index], --i) {
    cell_t cell = {index % board->cols, index / board->cols};
    board->path[i] = cell;
  }
  return true;
}

static click_result_t move_selection_to(board_t* board, cell_t target) {
  if (!board->has_selection || !contains(board, target) ||
      !find_path(board, board->selection, target)) {
    return CLICK_IGNORED;
  }
  set_board_value(board, target, board_value(board, board->selection));
  set_board_value(board, board->selection, EMPTY_CELL);
  board->has_selection = false;
  board->move_pending = true;
  return CLICK_MOVED;
}

// Gathers in queue the cells holding a value that are joined to a cell
// through neighbours holding it too, the cell first and counted whatever it
// holds. Stops early once it has found limit of them, unless limit is 0.
static int gather_group(board_t* board, cell_t cell, int value, int limit) {
  for (int i = 0; i < cell_count(board); ++i) {
    board->came_from[i] = UNVISITED;
  }
  int start = cell_index(board, cell);
  int head = 0, tail = 0;
  board->queue[tail++] = start;
  board->came_from[start] = start;
  while (head < tail && (limit == 0 || tail < limit)) {
    int index = board->queue[head++];
    cell_t member = {index % board->cols, index / board->cols};
    for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
      cell_t next = hex_neighbour(member, direction);
      if (!contains(board, next) || board_value(board, next) != value) {
        continue;
      }
      int next_index = cell_index(board, next);
      if (board->came_from[next_index] == UNVISITED) {
        board->came_from[next_index] = index;
        board->queue[tail++] = next_index;
      }
    }
  }
  return tail;
}

int board_group_size(board_t* board, cell_t cell) {
  int value = board_value(board, cell);
  return is_hexagon(value) ? gather_group(board, cell, value, 0) : 0;
}

static int largest_power_of_two_up_to(int number) {
  int power = 1;
  while (power * 2 <= number) {
    power *= 2;
  }
  return power;
}

settle_result_t settle_board_move(board_t* board, random_source_t* random) {
  if (!board->move_pending) {
    return SETTLED_NOTHING;
  }
  board->move_pending = false;
  cell_t moved = board->path[board->path_length - 1];
  int value = board_value(board, moved);
  int size = gather_group(board, moved, value, 0);
  board->merged_count = 0;
  board->merged_value = value;
  if (size < board->min_group_size) {
    populate_board(board, random, board->spawn_count);
    return SETTLED_SPAWN;
  }
  int merged_value = largest_power_of_two_up_to(value * size);
  bool walled = merged_value > MAX_HEXAGON_VALUE;
  // The moved hexagon is first in the queue; the rest are the others
  for (int i = 1; i < size; ++i) {
    int index = board->queue[i];
    cell_t cell = {index % board->cols, index / board->cols};
    board->merged[board->merged_count++] = cell;
    board->values[index] = walled ? WALL_CELL : EMPTY_CELL;
  }
  set_board_value(board, moved, walled ? WALL_CELL : merged_value);
  return walled ? SETTLED_WALL : SETTLED_MERGE;
}

click_result_t click_board_cell(board_t* board, cell_t cell) {
  int value = board_value(board, cell);
  if (value == EMPTY_CELL) {
    return move_selection_to(board, cell);
  }
  if (value == WALL_CELL) {
    return CLICK_IGNORED;
  }
  if (is_selected(board, cell)) {
    board->has_selection = false;
    return CLICK_UNSELECTED;
  }
  board->has_selection = true;
  board->selection = cell;
  return CLICK_SELECTED;
}

bool board_has_move(const board_t* board) {
  for (int index = 0; index < cell_count(board); ++index) {
    if (!is_hexagon(board->values[index])) {
      continue;
    }
    cell_t cell = {index % board->cols, index / board->cols};
    for (int direction = 0; direction < HEX_DIRECTION_COUNT; ++direction) {
      cell_t neighbour = hex_neighbour(cell, direction);
      if (contains(board, neighbour) &&
          board_value(board, neighbour) == EMPTY_CELL) {
        return true;
      }
    }
  }
  return false;
}

void reset_board(board_t* board) {
  for (int i = 0; i < cell_count(board); ++i) {
    board->values[i] = EMPTY_CELL;
  }
  board->has_selection = false;
  board->move_pending = false;
  board->path_length = 0;
  board->merged_count = 0;
}

int min_group_size_for(int cells) {
  // Halved, rounded and doubled: the nearest even number
  int size =
      2 * (int)lround(MIN_GROUP_SIZE * sqrt(cells / ORDINARY_CELL_COUNT) / 2);
  return size < MIN_GROUP_SIZE ? MIN_GROUP_SIZE : size;
}

int spawn_hexagon_count(int cells) {
  int count = (int)lround(cells * SPAWN_FRACTION);
  return count < 1 ? 1 : count;
}

int initial_hexagon_count(int cells) {
  return (int)lround(cells * INITIAL_FILL_FRACTION);
}

// Index of the nth empty cell, counting from zero
static int nth_empty_cell(const board_t* board, int n) {
  for (int i = 0; i < cell_count(board); ++i) {
    if (board->values[i] == EMPTY_CELL && n-- == 0) {
      return i;
    }
  }
  return -1;
}

// Puts a new hexagon on the first empty cell, counting from a random one,
// that takes a number without completing a group; false if none does
static bool add_hexagon(board_t* board, random_source_t* random, int empty) {
  int first_cell = random_below(random, empty);
  int first_value = random_below(random, NEW_HEXAGON_VALUE_COUNT);
  for (int i = 0; i < empty; ++i) {
    int index = nth_empty_cell(board, (first_cell + i) % empty);
    cell_t cell = {index % board->cols, index / board->cols};
    for (int j = 0; j < NEW_HEXAGON_VALUE_COUNT; ++j) {
      int value = 1 << ((first_value + j) % NEW_HEXAGON_VALUE_COUNT);
      if (gather_group(board, cell, value, board->min_group_size) <
          board->min_group_size) {
        board->values[index] = value;
        return true;
      }
    }
  }
  return false;
}

int populate_board(board_t* board, random_source_t* random, int count) {
  int empty = count_cells(board, is_empty);
  int added = 0;
  while (added < count && empty > 0 && add_hexagon(board, random, empty)) {
    --empty;
    ++added;
  }
  return added;
}
