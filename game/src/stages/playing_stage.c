#include "playing_stage.h"

#include <stdbool.h>
#include <stdlib.h>

#include "events.h"
#include "frame.h"
#include "frame_limiter.h"
#include "game.h"
#include "graphics.h"
#include "keyboard.h"
#include "stage.h"

/* ---- ==== ---- ==== init ==== ---- ==== ---- */

playing_stage_state_ptr create_playing_stage(game_ptr game) {
  playing_stage_state_ptr state = calloc(1, sizeof(playing_stage_state_t));
  if (!state) {
    return NULL;
  }
  state->game = game;
  state->graphics_context = &game->graphics_context;
  return state;
}

void destroy_playing_stage(playing_stage_state_ptr state) { free(state); }

/* ---- ==== ---- ==== main game loop ==== ---- ==== ---- */

static void render_playing_stage(playing_stage_state_ptr state) {
  clear_frame(state->graphics_context);
  render_frame(state->graphics_context);
}

game_stage_action_t handle_playing_stage(playing_stage_state_ptr state) {
  frame_limiter_t limiter = create_frame_limiter(state->game->settings.fps);
  while (true) {
    frame_limiter_wait(&limiter);
    if (drain_events() == QUIT_EVENT) {
      return QUIT;
    }
    keyboard_state_ptr keyboard = &state->game->keyboard_state;
    if (is_esc_key_pressed(keyboard)) {
      return QUIT;
    }
    if (is_f11_key_pressed(keyboard)) {
      toggle_fullscreen(state->graphics_context);
    }
    render_playing_stage(state);
  }
}

/* ---- ==== Stage Interface Implementation ==== ---- */

static void playing_init(stage_ptr stage, game_ptr game) {
  playing_stage_state_ptr state = create_playing_stage(game);
  stage->state = state;
}

static game_stage_action_t playing_update(stage_ptr stage) {
  playing_stage_state_ptr state = (playing_stage_state_ptr)stage->state;
  return handle_playing_stage(state);
}

static void playing_cleanup(stage_ptr stage) {
  playing_stage_state_ptr state = (playing_stage_state_ptr)stage->state;
  destroy_playing_stage(state);
  stage->state = NULL;
}

stage_ptr create_playing_stage_instance(void) {
  stage_ptr stage = malloc(sizeof(stage_t));
  if (!stage) {
    return NULL;
  }
  stage->state = NULL;
  stage->init = playing_init;
  stage->update = playing_update;
  stage->cleanup = playing_cleanup;
  stage->name = "PLAYING";
  return stage;
}
