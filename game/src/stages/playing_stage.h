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

#include "board.h"
#include "game.h"
#include "hex_grid.h"
#include "random_source.h"

typedef struct {
  game_ptr game;
  graphics_context_ptr graphics_context;
  hex_grid_t grid;
  board_t board;
  random_source_t random;
  bool has_hovered_cell;
  cell_t hovered_cell;
  bool button_was_down;
} playing_stage_state_t;

typedef playing_stage_state_t* playing_stage_state_ptr;

playing_stage_state_ptr create_playing_stage(game_ptr game);
void destroy_playing_stage(playing_stage_state_ptr state);

/**
 * @brief Tell the stage where on the screen the mouse is and whether its
 * button is down
 *
 * Call once per frame. A click happens when the button goes down, however
 * long it is then held.
 */
void move_playing_stage_pointer(playing_stage_state_ptr state, double x,
                                double y, bool button_down);

game_stage_action_t handle_playing_stage(playing_stage_state_ptr state);

#endif  // GAME_SRC_STAGES_PLAYING_STAGE_H_
