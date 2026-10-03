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
#define WALL_CELL (-1)
#define MAX_NEW_HEXAGON_VALUE 8
#define MAX_HEXAGON_VALUE 1024
// How many equal hexagons must touch to merge
#define MIN_GROUP_SIZE 4

typedef struct {
  int cols;
  int rows;
  int* values;  // Row by row; EMPTY_CELL, WALL_CELL or a hexagon's number
  bool has_selection;
  cell_t selection;  // The hexagon the player picked; valid if has_selection
  cell_t* path;      // Cells the last moved hexagon went through, ends included
  int path_length;
  int spawn_count;    // New hexagons after each move that merges nothing
  bool move_pending;  // A hexagon was moved and the move is not settled yet
  cell_t* merged;     // Cells emptied, or turned to wall, by the last merge
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
int board_wall_count(const board_t* board);

/**
 * @brief How many hexagons carry the same number as the one on a cell and
 * are joined to it through neighbours that do, itself included; 0 if the
 * cell holds no hexagon
 */
int board_group_size(board_t* board, cell_t cell);

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
 * Walls can be neither selected nor crossed.
 */
click_result_t click_board_cell(board_t* board, cell_t cell);

/** @brief How many hexagons a game starts with on a board of this size */
typedef enum {
  SETTLED_NOTHING,  // No move was waiting to be settled
  SETTLED_MERGE,
  SETTLED_WALL,
  SETTLED_SPAWN,
} settle_result_t;

/**
 * @brief Apply the consequences of the last move, once it has been shown
 *
 * If the moved hexagon now touches a group of equal numbers that makes at
 * least MIN_GROUP_SIZE with it, whatever its shape, every other hexagon in
 * the group is removed and the moved one carries their sum, rounded down to
 * a power of two. The cells emptied are left in merged.
 *
 * If that number would pass MAX_HEXAGON_VALUE, every hexagon in the group,
 * the moved one included, turns to wall for good instead.
 *
 * If nothing merged, spawn_count new hexagons appear, or as many as fit.
 */
settle_result_t settle_board_move(board_t* board, random_source_t* random);

/**
 * @brief Whether any hexagon has an empty cell next to it to move to
 *
 * When none has, the game is over.
 */
bool board_has_move(const board_t* board);

/** @brief Empty the board for a new game */
void reset_board(board_t* board);

/** @brief How many hexagons appear after a move that merges nothing */
int spawn_hexagon_count(int cell_count);

int initial_hexagon_count(int cell_count);

/**
 * @brief Add hexagons on random empty cells, each carrying a random power of
 * two up to MAX_NEW_HEXAGON_VALUE
 *
 * A new hexagon never completes a group that would merge: its cell and
 * number are chosen so that fewer than MIN_GROUP_SIZE equal numbers touch.
 *
 * @return How many were added: fewer than asked when the board fills up or
 * no number fits in any empty cell
 */
int populate_board(board_t* board, random_source_t* random, int count);

#endif  // GAME_SRC_BOARD_BOARD_H_
