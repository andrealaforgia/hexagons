#include "game.h"

#include <stdlib.h>

#include "game_settings.h"
#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"

game_t init_game(game_settings_t game_settings) {
  game_t game;
  game.settings = game_settings;
  game.graphics_context =
      init_graphics_context(game.settings.display, game.settings.display_mode,
                            game.settings.window_mode, game.settings.vsync);
  game.keyboard_state = init_keyboard_state();
  game.mouse_state = init_mouse_state();
  game.seed = (unsigned)rand();
  return game;
}

void terminate_game(const game_ptr game) {
  terminate_graphics_context(&game->graphics_context);
}
