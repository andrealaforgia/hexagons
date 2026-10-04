/**
 * @file game_options.h
 * @brief Command-line options of the game itself
 *
 * The engine reads the options it knows and rejects the rest, so the game
 * takes its own out of the command line first.
 */

#ifndef GAME_SRC_MAIN_GAME_OPTIONS_H_
#define GAME_SRC_MAIN_GAME_OPTIONS_H_

#include <stdbool.h>

// Let the size of the board decide how many hexagons make a group
#define AUTOMATIC_MIN_GROUP 0
// The fewest that can: the moved hexagon and one more
#define SMALLEST_MIN_GROUP 2

typedef struct {
  bool valid;     // false if an option of the game was malformed
  int min_group;  // AUTOMATIC_MIN_GROUP unless given
} game_options_t;

/**
 * @brief Read the game's options and remove them from the command line
 *
 * Leaves argc and argv holding what is left, in the same order. If an
 * option is malformed, the result is not valid and an error has been
 * printed.
 */
game_options_t take_game_options(int* argc, char* argv[]);

void print_game_help(void);

#endif  // GAME_SRC_MAIN_GAME_OPTIONS_H_
