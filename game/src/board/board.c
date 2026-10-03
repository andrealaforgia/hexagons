#include "board.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hex_grid.h"
#include "random_source.h"

#define INITIAL_FILL_FRACTION 0.10
#define SPAWN_FRACTION 0.01
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

int board_hexagon_count(const board_t* board) {
  int count = 0;
  for (int i = 0; i < cell_count(board); ++i) {
    if (board->values[i] != EMPTY_CELL) {
      ++count;
    }
  }
  return count;
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

// Empties the run of cells holding the value that starts next to a cell and
// goes in one direction, if asked to; returns how long the run is
static int equal_run(board_t* board, cell_t cell, int direction, int value,
                     bool remove) {
  int length = 0;
  cell_t next = hex_neighbour(cell, direction);
  while (board_value(board, next) == value) {
    if (remove) {
      set_board_value(board, next, EMPTY_CELL);
      board->merged[board->merged_count++] = next;
    }
    ++length;
    next = hex_neighbour(next, direction);
  }
  return length;
}

settle_result_t settle_board_move(board_t* board, random_source_t* random) {
  if (!board->move_pending) {
    return SETTLED_NOTHING;
  }
  board->move_pending = false;
  cell_t moved = board->path[board->path_length - 1];
  int value = board_value(board, moved);
  board->merged_count = 0;
  board->merged_value = value;
  // A line runs both ways from the moved hexagon along each of three axes
  for (int direction = 0; direction < HEX_DIRECTION_COUNT / 2; ++direction) {
    int opposite = direction + HEX_DIRECTION_COUNT / 2;
    int length = 1 + equal_run(board, moved, direction, value, false) +
                 equal_run(board, moved, opposite, value, false);
    if (length >= MIN_LINE_LENGTH) {
      equal_run(board, moved, direction, value, true);
      equal_run(board, moved, opposite, value, true);
    }
  }
  if (board->merged_count == 0) {
    populate_board(board, random, board->spawn_count);
    return SETTLED_SPAWN;
  }
  set_board_value(board, moved, value * MERGE_MULTIPLIER);
  return SETTLED_MERGE;
}

click_result_t click_board_cell(board_t* board, cell_t cell) {
  if (board_value(board, cell) == EMPTY_CELL) {
    return move_selection_to(board, cell);
  }
  if (is_selected(board, cell)) {
    board->has_selection = false;
    return CLICK_UNSELECTED;
  }
  board->has_selection = true;
  board->selection = cell;
  return CLICK_SELECTED;
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

int populate_board(board_t* board, random_source_t* random, int count) {
  int empty = cell_count(board) - board_hexagon_count(board);
  int added = 0;
  while (added < count && empty > 0) {
    int index = nth_empty_cell(board, random_below(random, empty));
    board->values[index] = 1 << random_below(random, NEW_HEXAGON_VALUE_COUNT);
    --empty;
    ++added;
  }
  return added;
}
