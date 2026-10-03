/**
 * @file game_font.h
 * @brief The font the game writes in
 */

#ifndef GAME_SRC_RENDERING_GAME_FONT_H_
#define GAME_SRC_RENDERING_GAME_FONT_H_

#define GAME_FONT_PATH "game/assets/fonts/PressStart2P.ttf"

/**
 * @brief Largest size not above the one asked at which the font is crisp
 *
 * The font is drawn on a grid of 8 pixels and is only crisp, and its glyphs
 * only exactly square, at multiples of that. Sizes too small for the grid
 * are left as they are, and never go below 1.
 */
int crisp_font_size(int size);

#endif  // GAME_SRC_RENDERING_GAME_FONT_H_
