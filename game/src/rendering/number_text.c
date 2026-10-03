#include "number_text.h"

#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "board.h"
#include "color.h"
#include "ttf_text.h"

#define NUMBER_FONT_PATH "game/assets/fonts/PressStart2P.ttf"

// The font is drawn on a grid of this many pixels and is only crisp, and
// only exactly square, at multiples of it
#define FONT_PIXEL_GRID 8

int number_font_size(int digits, double box_width, double box_height) {
  int size = (int)floor(fmin(box_width / digits, box_height));
  if (size >= FONT_PIXEL_GRID) {
    return size - size % FONT_PIXEL_GRID;
  }
  return size < 1 ? 1 : size;
}

number_text_t load_number_text(double box_width, double box_height) {
  number_text_t numbers = {0};
  for (int digits = 1; digits <= MAX_NUMBER_DIGITS; ++digits) {
    numbers.fonts[digits - 1] = load_ttf_font(
        NUMBER_FONT_PATH, number_font_size(digits, box_width, box_height));
  }
  return numbers;
}

void free_number_text(number_text_t* numbers) {
  for (int value = 0; value <= MAX_HEXAGON_VALUE; ++value) {
    if (numbers->textures[value]) {
      SDL_DestroyTexture(numbers->textures[value]);
      numbers->textures[value] = NULL;
    }
  }
  for (int i = 0; i < MAX_NUMBER_DIGITS; ++i) {
    free_ttf_font(numbers->fonts[i]);
    numbers->fonts[i] = NULL;
  }
}

// Writes the number and returns its font, or NULL if it has none
static ttf_font_t number_text(const number_text_t* numbers, int value,
                              char* text, size_t size) {
  if (value < 1 || value > MAX_HEXAGON_VALUE) {
    return NULL;
  }
  int digits = snprintf(text, size, "%d", value);
  return numbers->fonts[digits - 1];
}

bool measure_number_text(const number_text_t* numbers, int value, int* width,
                         int* height) {
  char text[MAX_NUMBER_DIGITS + 1];
  ttf_font_t font = number_text(numbers, value, text, sizeof(text));
  return font && get_ttf_text_size(font, text, width, height);
}

bool render_number_text(const graphics_context_ptr graphics_context,
                        number_text_t* numbers, int value, point_t centre,
                        color_t color) {
  char text[MAX_NUMBER_DIGITS + 1];
  ttf_font_t font = number_text(numbers, value, text, sizeof(text));
  if (!font) {
    return false;
  }
  if (!numbers->textures[value]) {
    // Written once in white, then tinted to the colour asked for
    SDL_Color white = {255, 255, 255, 255};
    numbers->textures[value] =
        render_ttf_text(graphics_context, font, text, white);
  }
  SDL_Texture* texture = numbers->textures[value];
  int width = 0, height = 0;
  if (!texture || SDL_QueryTexture(texture, NULL, NULL, &width, &height) != 0) {
    return false;
  }
  SDL_SetTextureColorMod(texture, R(color), G(color), B(color));
  SDL_Rect destination = {(int)lround(centre.x - width / 2.0),
                          (int)lround(centre.y - height / 2.0), width, height};
  return SDL_RenderCopy(graphics_context->renderer, texture, NULL,
                        &destination) == 0;
}
