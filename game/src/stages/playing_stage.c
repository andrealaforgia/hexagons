#include "playing_stage.h"

#include <stdbool.h>
#include <stdlib.h>

#include "board.h"
#include "board_renderer.h"
#include "events.h"
#include "frame.h"
#include "frame_limiter.h"
#include "game.h"
#include "game_constants.h"
#include "graphics.h"
#include "hex_grid.h"
#include "keyboard.h"
#include "mouse.h"
#include "random_source.h"
#include "stage.h"

/* ---- ==== ---- ==== init ==== ---- ==== ---- */

playing_stage_state_ptr create_playing_stage(game_ptr game) {
  playing_stage_state_ptr state = calloc(1, sizeof(playing_stage_state_t));
  if (!state) {
    return NULL;
  }
  state->game = game;
  state->graphics_context = &game->graphics_context;
  int screen_height = state->graphics_context->screen_height;
  state->grid =
      create_hex_grid(state->graphics_context->screen_width, screen_height,
                      screen_height * HEX_RADIUS_SCREEN_FRACTION);
  if (!init_board(&state->board, state->grid.cols, state->grid.rows)) {
    destroy_playing_stage(state);
    return NULL;
  }
  state->random = create_random_source(game->seed);
  populate_board(&state->board, &state->random,
                 initial_hexagon_count(hex_grid_cell_count(&state->grid)));
  return state;
}

void destroy_playing_stage(playing_stage_state_ptr state) {
  if (state != NULL) {
    destroy_board(&state->board);
    free(state);
  }
}

/* ---- ==== ---- ==== input ==== ---- ==== ---- */

void point_playing_stage_at(playing_stage_state_ptr state, double x, double y) {
  state->has_hovered_cell =
      hex_cell_at(&state->grid, x, y, &state->hovered_cell);
}

// The mouse reports window coordinates, which differ from screen coordinates
// when the window is not the size of the display mode (high-DPI displays).
static void track_mouse(playing_stage_state_ptr state) {
  mouse_state_ptr mouse = &state->game->mouse_state;
  update_mouse_state(mouse);
  int window_width = 0, window_height = 0;
  get_window_size(state->graphics_context->window, &window_width,
                  &window_height);
  if (window_width <= 0 || window_height <= 0) {
    return;
  }
  point_playing_stage_at(
      state,
      get_mouse_x(mouse) * (double)state->graphics_context->screen_width /
          window_width,
      get_mouse_y(mouse) * (double)state->graphics_context->screen_height /
          window_height);
}

/* ---- ==== ---- ==== main game loop ==== ---- ==== ---- */

static void render_playing_stage(playing_stage_state_ptr state) {
  clear_frame(state->graphics_context);
  render_board(state->graphics_context, &state->grid, &state->board,
               state->has_hovered_cell ? &state->hovered_cell : NULL);
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
    track_mouse(state);
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
