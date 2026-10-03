/**
 * @file game_constants.h
 * @brief Tunable constants for the playfield and its appearance
 */

#ifndef GAME_SRC_MAIN_GAME_CONSTANTS_H_
#define GAME_SRC_MAIN_GAME_CONSTANTS_H_

// The hexagon radius is this fraction of the screen height
#define HEX_RADIUS_SCREEN_FRACTION (1.0 / 24.0)

// Hexagons are drawn slightly smaller than their cell to leave a gap
#define HEX_DRAWN_RADIUS_FRACTION 0.9

#define EMPTY_CELL_BORDER_COLOR 0x303030
#define HOVERED_CELL_BORDER_COLOR 0xA0A0A0
#define HOVERED_CELL_FILL_COLOR 0x202020

#endif  // GAME_SRC_MAIN_GAME_CONSTANTS_H_
