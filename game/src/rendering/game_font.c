#include "game_font.h"

#define FONT_PIXEL_GRID 8

int crisp_font_size(int size) {
  if (size >= FONT_PIXEL_GRID) {
    return size - size % FONT_PIXEL_GRID;
  }
  return size < 1 ? 1 : size;
}
