#include "hand_cursor.h"

#include <SDL.h>
#include <stdint.h>

// One pixel per character: X is outline, a dot is fill
static const char HAND[HAND_CURSOR_HEIGHT][HAND_CURSOR_WIDTH + 1] = {
    "     XX          ", "    X..X         ", "    X..X         ",
    "    X..X         ", "    X..X         ", "    X..XXX       ",
    "    X..X..XXX    ", "    X..X..X..XX  ", "    X..X..X..X.X ",
    "XXX X..X..X..X..X", "X..XX...........X", "X...X...........X",
    " X..............X", "  X.............X", "  X.............X",
    "   X............X", "   X...........X ", "    X..........X ",
    "    X..........X ", "     X........X  ", "     X........X  ",
    "     XXXXXXXXXX  ",
};

// The hand is this fraction of the height of the window, at least
#define WINDOW_HEIGHT_PER_SCALE_STEP 300
#define MIN_SCALE 2

int hand_cursor_scale(int window_height) {
  int scale = window_height / WINDOW_HEIGHT_PER_SCALE_STEP;
  return scale < MIN_SCALE ? MIN_SCALE : scale;
}

uint32_t hand_cursor_pixel(int x, int y, int scale) {
  if (x < 0 || y < 0 || x >= HAND_CURSOR_WIDTH * scale ||
      y >= HAND_CURSOR_HEIGHT * scale) {
    return HAND_CURSOR_CLEAR;
  }
  switch (HAND[y / scale][x / scale]) {
    case 'X':
      return HAND_CURSOR_OUTLINE;
    case '.':
      return HAND_CURSOR_FILL;
    default:
      return HAND_CURSOR_CLEAR;
  }
}

SDL_Cursor* show_hand_cursor(int scale) {
  int width = HAND_CURSOR_WIDTH * scale, height = HAND_CURSOR_HEIGHT * scale;
  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
      0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
  SDL_Cursor* cursor = NULL;
  if (surface) {
    for (int y = 0; y < height; ++y) {
      uint32_t* row =
          (uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch);
      for (int x = 0; x < width; ++x) {
        row[x] = hand_cursor_pixel(x, y, scale);
      }
    }
    // Point with the middle of the enlarged tip
    cursor =
        SDL_CreateColorCursor(surface, HAND_CURSOR_HOT_X * scale + scale / 2,
                              HAND_CURSOR_HOT_Y * scale);
    SDL_FreeSurface(surface);
  }
  if (cursor) {
    SDL_SetCursor(cursor);
  }
  SDL_ShowCursor(SDL_ENABLE);
  return cursor;
}
