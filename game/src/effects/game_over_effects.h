/**
 * @file game_over_effects.h
 * @brief How the game over display moves with time
 */

#ifndef GAME_SRC_EFFECTS_GAME_OVER_EFFECTS_H_
#define GAME_SRC_EFFECTS_GAME_OVER_EFFECTS_H_

#include <stdbool.h>

/**
 * @brief Whether the flashing title is on
 * @param seconds Time since the game ended
 */
bool is_game_over_title_shown(double seconds);

/**
 * @brief How far the prompt has drifted from its place, up or down
 * @param seconds Time since the game ended
 * @param amplitude The furthest it drifts either way
 */
double game_over_prompt_offset(double seconds, double amplitude);

#endif  // GAME_SRC_EFFECTS_GAME_OVER_EFFECTS_H_
