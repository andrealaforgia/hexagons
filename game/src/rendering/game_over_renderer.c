#include "game_over_renderer.h"

#include <SDL.h>
#include <math.h>

#include "game_constants.h"
#include "game_font.h"
#include "game_over_effects.h"
#include "graphics.h"
#include "text.h"
#include "ttf_text.h"

#define TITLE "GAME OVER"
#define PROMPT "Press space to restart or ESC to exit"

static int title_size(int screen_height) {
  return crisp_font_size((int)(screen_height * GAME_OVER_TITLE_FRACTION));
}

static int prompt_size(int screen_height) {
  return crisp_font_size((int)(screen_height * GAME_OVER_PROMPT_FRACTION));
}

game_over_text_t load_game_over_text(int screen_height) {
  game_over_text_t text = {0};
  text.title_font = load_ttf_font(GAME_FONT_PATH, title_size(screen_height));
  text.prompt_font = load_ttf_font(GAME_FONT_PATH, prompt_size(screen_height));
  return text;
}

void free_game_over_text(game_over_text_t* text) {
  if (text->title) {
    SDL_DestroyTexture(text->title);
  }
  if (text->prompt) {
    SDL_DestroyTexture(text->prompt);
  }
  free_ttf_font(text->title_font);
  free_ttf_font(text->prompt_font);
  game_over_text_t none = {0};
  *text = none;
}

// Writes the text centred on a point in the game's font, keeping it as a
// texture for next time; false if the font is not there
static bool render_label(const graphics_context_ptr graphics_context,
                         ttf_font_t font, SDL_Texture** texture,
                         const char* text, point_t centre, color_t color) {
  if (!font) {
    return false;
  }
  if (!*texture) {
    // Written once in white, then tinted to the colour asked for
    SDL_Color white = {255, 255, 255, 255};
    *texture = render_ttf_text(graphics_context, font, text, white);
  }
  int width = 0, height = 0;
  if (!*texture ||
      SDL_QueryTexture(*texture, NULL, NULL, &width, &height) != 0) {
    return false;
  }
  SDL_SetTextureColorMod(*texture, R(color), G(color), B(color));
  SDL_Rect destination = {(int)lround(centre.x - width / 2.0),
                          (int)lround(centre.y - height / 2.0), width, height};
  return SDL_RenderCopy(graphics_context->renderer, *texture, NULL,
                        &destination) == 0;
}

// Without the font, fall back on the engine's line-drawn text
static void render_line_drawn(const graphics_context_ptr graphics_context,
                              const char* text, point_t centre, int height,
                              color_t color) {
  int scale = height / 3 > 0 ? height / 3 : 1;
  text_dimensions_t dimensions = calculate_text_dimensions(text, scale);
  write_text(graphics_context, text,
             point(centre.x - dimensions.width / 2.0,
                   centre.y + dimensions.height / 2.0),
             scale, color);
}

void render_game_over(const graphics_context_ptr graphics_context,
                      game_over_text_t* text, double seconds) {
  int width = graphics_context->screen_width;
  int height = graphics_context->screen_height;
  int title_height = title_size(height), prompt_height = prompt_size(height);
  point_t centre = point(width / 2.0, height / 2.0);

  // A dark band so that the text stands out from the hexagons behind it
  int band_height = title_height * 3 + prompt_height * 4;
  draw_filled_rect_alpha(graphics_context, 0,
                         (int)centre.y - title_height * 3 / 2, width,
                         band_height, COLOR_BLACK, GAME_OVER_BAND_ALPHA);

  if (is_game_over_title_shown(seconds)) {
    if (!render_label(graphics_context, text->title_font, &text->title, TITLE,
                      centre, GAME_OVER_TITLE_COLOR)) {
      render_line_drawn(graphics_context, TITLE, centre, title_height,
                        GAME_OVER_TITLE_COLOR);
    }
  }

  point_t prompt_centre = point(
      centre.x, centre.y + title_height * 1.5 + prompt_height +
                    game_over_prompt_offset(seconds, prompt_height / 2.0));
  if (!render_label(graphics_context, text->prompt_font, &text->prompt, PROMPT,
                    prompt_centre, GAME_OVER_PROMPT_COLOR)) {
    render_line_drawn(graphics_context, PROMPT, prompt_centre, prompt_height,
                      GAME_OVER_PROMPT_COLOR);
  }
}
