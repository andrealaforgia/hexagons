/**
 * @file game_over_renderer.h
 * @brief Draws the game over display over the playfield
 */

#ifndef GAME_SRC_RENDERING_GAME_OVER_RENDERER_H_
#define GAME_SRC_RENDERING_GAME_OVER_RENDERER_H_

#include <SDL.h>

#include "graphics_context.h"
#include "ttf_text.h"

typedef struct {
  ttf_font_t title_font;  // NULL if not loaded
  ttf_font_t prompt_font;
  SDL_Texture* title;  // Made the first time they are drawn
  SDL_Texture* prompt;
} game_over_text_t;

/**
 * @brief Load the font at sizes that suit the screen
 *
 * The TTF system must be initialised. If the font cannot be loaded the
 * result is still safe to use: the text is line-drawn instead.
 */
game_over_text_t load_game_over_text(int screen_height);
void free_game_over_text(game_over_text_t* text);

/**
 * @brief Flash "GAME OVER" at the centre of the screen, with the prompt to
 * restart or exit drifting gently below it
 * @param seconds Time since the game ended
 */
void render_game_over(const graphics_context_ptr graphics_context,
                      game_over_text_t* text, double seconds);

#endif  // GAME_SRC_RENDERING_GAME_OVER_RENDERER_H_
