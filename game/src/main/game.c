#include "game.h"

#include <SDL.h>
#include <stdlib.h>

#include "game_constants.h"
#include "game_settings.h"
#include "graphics.h"
#include "hand_cursor.h"
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
  // The engine hides the pointer; this game is played with it. Its size
  // follows the window, which is what the pointer is measured against.
  int window_width = 0, window_height = 0;
  if (game.graphics_context.window) {
    get_window_size(game.graphics_context.window, &window_width,
                    &window_height);
  }
  game.cursor = show_hand_cursor(hand_cursor_scale(window_height));
  game.seed = (unsigned)rand();
  number_text_t no_number_text = {0};
  game.number_text = no_number_text;
  game_over_text_t no_game_over_text = {0};
  game.game_over_text = no_game_over_text;
  if (init_ttf_system()) {
    double inner_radius =
        game.graphics_context.screen_height * HEX_RADIUS_SCREEN_FRACTION *
        HEX_DRAWN_RADIUS_FRACTION * (1 - HEX_BORDER_THICKNESS_FRACTION);
    game.number_text =
        load_number_text(inner_radius * HEX_NUMBER_WIDTH_FRACTION,
                         inner_radius * HEX_NUMBER_HEIGHT_FRACTION);
    game.game_over_text =
        load_game_over_text(game.graphics_context.screen_height);
  }
  return game;
}

void terminate_game(const game_ptr game) {
  if (game->cursor) {
    SDL_FreeCursor(game->cursor);
    game->cursor = NULL;
  }
  free_number_text(&game->number_text);
  free_game_over_text(&game->game_over_text);
  quit_ttf_system();
  terminate_graphics_context(&game->graphics_context);
}
