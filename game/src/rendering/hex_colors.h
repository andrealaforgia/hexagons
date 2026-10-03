/**
 * @file hex_colors.h
 * @brief Colours of the hexagons, chosen by the number they carry
 */

#ifndef GAME_SRC_RENDERING_HEX_COLORS_H_
#define GAME_SRC_RENDERING_HEX_COLORS_H_

#include "color.h"

/**
 * @brief Border colour of a hexagon: every number has its own
 */
color_t hex_border_color(int value);

/**
 * @brief Fill of a selected hexagon: a light tone of its border colour
 */
color_t hex_selected_fill_color(int value);

#endif  // GAME_SRC_RENDERING_HEX_COLORS_H_
