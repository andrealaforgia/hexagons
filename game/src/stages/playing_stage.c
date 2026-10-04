#include "playing_stage.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "board.h"
#include "board_renderer.h"
#include "debris.h"
#include "debris_renderer.h"
#include "events.h"
#include "frame.h"
#include "frame_limiter.h"
#include "game.h"
#include "game_constants.h"
#include "game_options.h"
#include "game_over_renderer.h"
#include "graphics.h"
#include "hex_colors.h"
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
  int cells = hex_grid_cell_count(&state->grid);
  if (!init_board(&state->board, state->grid.cols, state->grid.rows) ||
      !init_debris(&state->debris, cells * DEBRIS_PIECES_PER_HEXAGON)) {
    destroy_playing_stage(state);
    return NULL;
  }
  if (game->settings.min_group != AUTOMATIC_MIN_GROUP) {
    state->board.min_group_size = game->settings.min_group;
  }
  state->random = create_random_source(game->seed);
  state->effects_random = create_random_source(game->seed);
  restart_playing_stage(state);
  return state;
}

void restart_playing_stage(playing_stage_state_ptr state) {
  reset_board(&state->board);
  clear_debris(&state->debris);
  populate_board(&state->board, &state->random,
                 initial_hexagon_count(hex_grid_cell_count(&state->grid)));
  state->travelling = false;
  state->game_over = !board_has_move(&state->board);
  state->game_over_seconds = 0;
}

bool is_playing_stage_over(const playing_stage_state_ptr state) {
  return state->game_over;
}

void destroy_playing_stage(playing_stage_state_ptr state) {
  if (state != NULL) {
    destroy_board(&state->board);
    destroy_debris(&state->debris);
    free(state);
  }
}

/* ---- ==== ---- ==== input ==== ---- ==== ---- */

void move_playing_stage_pointer(playing_stage_state_ptr state, double x,
                                double y, bool button_down) {
  state->has_hovered_cell =
      hex_cell_at(&state->grid, x, y, &state->hovered_cell);
  bool clicked = button_down && !state->button_was_down;
  state->button_was_down = button_down;
  if (clicked && state->has_hovered_cell && !state->travelling &&
      !state->game_over) {
    if (click_board_cell(&state->board, state->hovered_cell) == CLICK_MOVED) {
      state->travelling = true;
      state->travel_progress = 0;
    }
  }
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
  move_playing_stage_pointer(
      state,
      get_mouse_x(mouse) * (double)state->graphics_context->screen_width /
          window_width,
      get_mouse_y(mouse) * (double)state->graphics_context->screen_height /
          window_height,
      is_mouse_left_button_pressed(mouse));
}

/* ---- ==== ---- ==== travel ==== ---- ==== ---- */

static int travel_steps(const playing_stage_state_ptr state) {
  return state->board.path_length - 1;
}

bool is_playing_stage_travelling(const playing_stage_state_ptr state) {
  return state->travelling;
}

// The hexagons a merge removed burst where they stood
static void burst_merged_hexagons(playing_stage_state_ptr state) {
  const board_t* board = &state->board;
  for (int i = 0; i < board->merged_count; ++i) {
    burst_hexagon(&state->debris, &state->effects_random,
                  hex_cell_centre(&state->grid, board->merged[i]),
                  state->grid.radius * HEX_DRAWN_RADIUS_FRACTION,
                  hex_border_color(board->merged_value));
  }
}

static void advance_travel(playing_stage_state_ptr state, double delta_time) {
  if (!state->travelling) {
    return;
  }
  state->travel_progress += delta_time * TRAVEL_STEPS_PER_SECOND / BASELINE_FPS;
  if (state->travel_progress >= travel_steps(state)) {
    state->travel_progress = travel_steps(state);
    state->travelling = false;
    if (settle_board_move(&state->board, &state->random) == SETTLED_MERGE) {
      burst_merged_hexagons(state);
    }
    state->game_over = !board_has_move(&state->board);
  }
}

void advance_playing_stage(playing_stage_state_ptr state, double delta_time) {
  if (!isfinite(delta_time) || delta_time <= 0) {
    return;
  }
  if (state->game_over) {
    state->game_over_seconds += delta_time / BASELINE_FPS;
  }
  advance_debris(&state->debris, delta_time,
                 state->graphics_context->screen_height);
  advance_travel(state, delta_time);
}

point_t travelling_hexagon_position(const playing_stage_state_ptr state) {
  const board_t* board = &state->board;
  int step = (int)floor(state->travel_progress);
  if (step >= travel_steps(state)) {
    return hex_cell_centre(&state->grid, board->path[travel_steps(state)]);
  }
  double fraction = state->travel_progress - step;
  point_t from = hex_cell_centre(&state->grid, board->path[step]);
  point_t to = hex_cell_centre(&state->grid, board->path[step + 1]);
  return point(from.x + (to.x - from.x) * fraction,
               from.y + (to.y - from.y) * fraction);
}

/* ---- ==== ---- ==== main game loop ==== ---- ==== ---- */

static void render_playing_stage(playing_stage_state_ptr state) {
  clear_frame(state->graphics_context);
  travelling_hexagon_t travelling;
  if (state->travelling) {
    travelling.destination = state->board.path[travel_steps(state)];
    travelling.position = travelling_hexagon_position(state);
  }
  render_board(state->graphics_context, &state->grid, &state->board,
               state->has_hovered_cell ? &state->hovered_cell : NULL,
               state->travelling ? &travelling : NULL,
               &state->game->number_text);
  render_debris(state->graphics_context, &state->debris);
  if (state->game_over) {
    render_game_over(state->graphics_context, &state->game->game_over_text,
                     state->game_over_seconds);
  }
  render_frame(state->graphics_context);
}

game_stage_action_t handle_playing_stage(playing_stage_state_ptr state) {
  frame_limiter_t limiter = create_frame_limiter(state->game->settings.fps);
  while (true) {
    double delta_time = frame_limiter_wait(&limiter);
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
    if (state->game_over && is_space_key_pressed(keyboard)) {
      restart_playing_stage(state);
    }
    track_mouse(state);
    advance_playing_stage(state, delta_time);
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
