/**
 * @file board.h
 * @brief Contents of the playfield and the rules that change them
 *
 * Holds what each cell of the hexagonal grid contains. Has no knowledge of
 * the screen: cells are addressed by column and row.
 */

#ifndef GAME_SRC_BOARD_BOARD_H_
#define GAME_SRC_BOARD_BOARD_H_

#include <stdbool.h>

#include "hex_grid.h"
#include "random_source.h"

#define EMPTY_CELL 0
#define MAX_NEW_HEXAGON_VALUE 8
#define MAX_HEXAGON_VALUE 1024
#define MIN_LINE_LENGTH 4
// A merge is worth this many times the number, however long the lines
#define MERGE_MULTIPLIER 4

typedef struct {
  int cols;
  int rows;
  int* values;  // Row by row; EMPTY_CELL or the number in the hexagon
  bool has_selection;
  cell_t selection;  // The hexagon the player picked; valid if has_selection
  cell_t* path;      // Cells the last moved hexagon went through, ends included
  int path_length;
  bool move_pending;  // A hexagon was moved and the move is not settled yet
  cell_t* merged;     // Cells emptied by the last merge
  int merged_count;
  int merged_value;  // The number the merged hexagons carried
  int* came_from;    // Working space for finding paths
  int* queue;
} board_t;

/**
 * @brief Allocate an empty board
 * @return false if the board could not be allocated
 */
bool init_board(board_t* board, int cols, int rows);
void destroy_board(board_t* board);

/**
 * @brief Number in the hexagon on a cell; EMPTY_CELL if there is none or the
 * cell is outside the board
 */
int board_value(const board_t* board, cell_t cell);

/** @brief Put a value on a cell; does nothing for cells outside the board */
void set_board_value(board_t* board, cell_t cell, int value);

int board_hexagon_count(const board_t* board);

typedef enum {
  CLICK_IGNORED,
  CLICK_SELECTED,
  CLICK_UNSELECTED,
  CLICK_MOVED,
} click_result_t;

/**
 * @brief Apply the player's click on a cell
 *
 * Clicking a hexagon selects it, replacing any earlier selection; clicking
 * the selected hexagon unselects it. Clicking an empty cell moves the
 * selected hexagon there by the shortest way through empty cells, leaving
 * that way in path and nothing selected. If there is no way, nothing changes.
 */
click_result_t click_board_cell(board_t* board, cell_t cell);

/** @brief How many hexagons a game starts with on a board of this size */
/**
 * @brief Apply the consequences of the last move, once it has been shown
 *
 * If the moved hexagon completed one or more straight lines of at least
 * MIN_LINE_LENGTH equal numbers, every other hexagon in those lines is
 * removed and the moved one carries MERGE_MULTIPLIER times its number. The
 * cells emptied are left in merged. Does nothing if no move is waiting to be
 * settled.
 *
 * @return true if hexagons merged
 */
bool settle_board_move(board_t* board);

int initial_hexagon_count(int cell_count);

/**
 * @brief Add hexagons on random empty cells, each carrying a random power of
 * two up to MAX_NEW_HEXAGON_VALUE
 * @return How many were added: fewer than asked when the board fills up
 */
int populate_board(board_t* board, random_source_t* random, int count);

#endif  // GAME_SRC_BOARD_BOARD_H_
