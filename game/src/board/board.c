#include "board.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hex_grid.h"
#include "random_source.h"

#define INITIAL_FILL_FRACTION 0.10

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
  board->cols = cols;
  board->rows = rows;
  board->has_selection = false;
  board->values = calloc((size_t)(cols * rows), sizeof(int));
  return board->values != NULL;
}

void destroy_board(board_t* board) {
  free(board->values);
  board->values = NULL;
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

void click_board_cell(board_t* board, cell_t cell) {
  if (board_value(board, cell) == EMPTY_CELL) {
    return;
  }
  if (is_selected(board, cell)) {
    board->has_selection = false;
    return;
  }
  board->has_selection = true;
  board->selection = cell;
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
    board->values[index] = 1 + random_below(random, MAX_NEW_HEXAGON_VALUE);
    --empty;
    ++added;
  }
  return added;
}
