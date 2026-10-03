/**
 * @file game.h
 * @brief Main game state and lifecycle management
 *
 * Defines the core game structure containing settings and the major
 * subsystems: graphics, keyboard and mouse. Manages game initialization
 * and termination.
 */

#ifndef GAME_SRC_MAIN_GAME_H_
#define GAME_SRC_MAIN_GAME_H_

#include "game_settings.h"
#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"
#include "number_text.h"

typedef enum { PROGRESS, QUIT } game_stage_action_t;

typedef struct {
  game_settings_t settings;
  graphics_context_t graphics_context;
  keyboard_state_t keyboard_state;
  mouse_state_t mouse_state;
  unsigned seed;  // Decides how the hexagons fall
  number_text_t number_text;
} game_t, *game_ptr;

game_t init_game(game_settings_t game_settings);
void terminate_game(const game_ptr game);

#endif  // GAME_SRC_MAIN_GAME_H_
