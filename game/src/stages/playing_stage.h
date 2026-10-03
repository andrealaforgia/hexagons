/**
 * @file playing_stage.h
 * @brief Main gameplay stage implementation
 *
 * Implements the gameplay loop: reads input, advances the game and renders
 * the playfield.
 */

#ifndef GAME_SRC_STAGES_PLAYING_STAGE_H_
#define GAME_SRC_STAGES_PLAYING_STAGE_H_

#include "game.h"

typedef struct {
  game_ptr game;
  graphics_context_ptr graphics_context;
} playing_stage_state_t;

typedef playing_stage_state_t* playing_stage_state_ptr;

playing_stage_state_ptr create_playing_stage(game_ptr game);
void destroy_playing_stage(playing_stage_state_ptr state);

game_stage_action_t handle_playing_stage(playing_stage_state_ptr state);

#endif  // GAME_SRC_STAGES_PLAYING_STAGE_H_
