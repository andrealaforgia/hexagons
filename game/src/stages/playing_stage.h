/**
 * @file playing_stage.h
 * @brief Main gameplay stage implementation
 *
 * Implements the gameplay loop: reads input, advances the game and renders
 * the playfield.
 */

#ifndef GAME_SRC_STAGES_PLAYING_STAGE_H_
#define GAME_SRC_STAGES_PLAYING_STAGE_H_

#include <stdbool.h>

#include "game.h"
#include "hex_grid.h"

typedef struct {
  game_ptr game;
  graphics_context_ptr graphics_context;
  hex_grid_t grid;
  bool has_hovered_cell;
  cell_t hovered_cell;
} playing_stage_state_t;

typedef playing_stage_state_t* playing_stage_state_ptr;

playing_stage_state_ptr create_playing_stage(game_ptr game);
void destroy_playing_stage(playing_stage_state_ptr state);

/**
 * @brief Tell the stage where on the screen the mouse is pointing
 */
void point_playing_stage_at(playing_stage_state_ptr state, double x, double y);

game_stage_action_t handle_playing_stage(playing_stage_state_ptr state);

#endif  // GAME_SRC_STAGES_PLAYING_STAGE_H_
