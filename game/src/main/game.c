#include "game.h"

#include <SDL.h>
#include <stdlib.h>

#include "game_constants.h"
#include "game_settings.h"
#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"
#include "number_text.h"
#include "ttf_text.h"

game_t init_game(game_settings_t game_settings) {
  game_t game;
  game.settings = game_settings;
  game.graphics_context =
      init_graphics_context(game.settings.display, game.settings.display_mode,
                            game.settings.window_mode, game.settings.vsync);
  game.keyboard_state = init_keyboard_state();
  game.mouse_state = init_mouse_state();
  // The engine hides the pointer; this game is played with it
  SDL_ShowCursor(SDL_ENABLE);
  game.seed = (unsigned)rand();
  number_text_t no_number_text = {0};
  game.number_text = no_number_text;
  if (init_ttf_system()) {
    double inner_radius =
        game.graphics_context.screen_height * HEX_RADIUS_SCREEN_FRACTION *
        HEX_DRAWN_RADIUS_FRACTION * (1 - HEX_BORDER_THICKNESS_FRACTION);
    game.number_text =
        load_number_text(inner_radius * HEX_NUMBER_WIDTH_FRACTION,
                         inner_radius * HEX_NUMBER_HEIGHT_FRACTION);
  }
  return game;
}

void terminate_game(const game_ptr game) {
  free_number_text(&game->number_text);
  quit_ttf_system();
  terminate_graphics_context(&game->graphics_context);
}
