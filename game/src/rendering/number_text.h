/**
 * @file number_text.h
 * @brief The numbers written on the hexagons, in the game's font
 *
 * Loads the font at one size per number of digits, so that every number
 * fills its hexagon as far as it can, and keeps each number as a texture
 * once it has been written.
 */

#ifndef GAME_SRC_RENDERING_NUMBER_TEXT_H_
#define GAME_SRC_RENDERING_NUMBER_TEXT_H_

#include <SDL.h>
#include <stdbool.h>

#include "board.h"
#include "color.h"
#include "geometry.h"
#include "graphics_context.h"
#include "ttf_text.h"

#define MAX_NUMBER_DIGITS 4

typedef struct {
  ttf_font_t fonts[MAX_NUMBER_DIGITS];  // By digit count; NULL if not loaded
  SDL_Texture* textures[MAX_HEXAGON_VALUE + 1];  // By number, made on demand
} number_text_t;

/**
 * @brief Largest font size at which a number of so many digits fits in a box
 *
 * Relies on the glyphs of the font being as wide as they are tall, which
 * holds at multiples of 8 pixels; the size is one of those whenever the box
 * has room for 8.
 */
int number_font_size(int digits, double box_width, double box_height);

/**
 * @brief Load the font at the sizes that fit numbers in a box
 *
 * The TTF system must be initialised. If the font cannot be loaded the
 * result is still safe to use: it measures and renders nothing.
 */
number_text_t load_number_text(double box_width, double box_height);
void free_number_text(number_text_t* numbers);

/** @return false if the number cannot be written */
bool measure_number_text(const number_text_t* numbers, int value, int* width,
                         int* height);

/**
 * @brief Write a number centred on a point
 * @return false if the number could not be written
 */
bool render_number_text(const graphics_context_ptr graphics_context,
                        number_text_t* numbers, int value, point_t centre,
                        color_t color);

#endif  // GAME_SRC_RENDERING_NUMBER_TEXT_H_
