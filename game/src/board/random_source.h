/**
 * @file random_source.h
 * @brief Seeded random numbers for the game rules
 *
 * The rules draw their randomness from here rather than from rand(), so
 * that a seed fully determines how a game unfolds.
 */

#ifndef GAME_SRC_BOARD_RANDOM_SOURCE_H_
#define GAME_SRC_BOARD_RANDOM_SOURCE_H_

#include <stdint.h>

typedef struct {
  uint32_t state;
} random_source_t;

random_source_t create_random_source(uint32_t seed);

/**
 * @brief Next random number in [0, bound); 0 when bound is not positive
 */
int random_below(random_source_t* random, int bound);

#endif  // GAME_SRC_BOARD_RANDOM_SOURCE_H_
