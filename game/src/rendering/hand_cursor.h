/**
 * @file hand_cursor.h
 * @brief The mouse pointer: a large pointing hand
 */

#ifndef GAME_SRC_RENDERING_HAND_CURSOR_H_
#define GAME_SRC_RENDERING_HAND_CURSOR_H_

#include <SDL.h>
#include <stdint.h>

#define HAND_CURSOR_WIDTH 17
#define HAND_CURSOR_HEIGHT 22

// The pixel that points: the tip of the finger
#define HAND_CURSOR_HOT_X 5
#define HAND_CURSOR_HOT_Y 0

// Pixels, as alpha, red, green, blue
#define HAND_CURSOR_CLEAR 0x00000000u
#define HAND_CURSOR_FILL 0xFFFFFFFFu
#define HAND_CURSOR_OUTLINE 0xFF000000u

/**
 * @brief How many times to enlarge the hand for a window of this height
 */
int hand_cursor_scale(int window_height);

/**
 * @brief A pixel of the hand enlarged that many times; clear outside it
 */
uint32_t hand_cursor_pixel(int x, int y, int scale);

/**
 * @brief Make the hand the mouse pointer and show it
 * @return The cursor, to be freed with SDL_FreeCursor; NULL if it could not
 * be made, in which case the system pointer is shown instead
 */
SDL_Cursor* show_hand_cursor(int scale);

#endif  // GAME_SRC_RENDERING_HAND_CURSOR_H_
