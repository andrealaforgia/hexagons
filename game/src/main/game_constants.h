/**
 * @file game_constants.h
 * @brief Tunable constants for the playfield and its appearance
 */

#ifndef GAME_SRC_MAIN_GAME_CONSTANTS_H_
#define GAME_SRC_MAIN_GAME_CONSTANTS_H_

// The hexagon radius is this fraction of the screen height
#define HEX_RADIUS_SCREEN_FRACTION (1.0 / 24.0)

// How fast a moved hexagon travels, in cells
#define TRAVEL_STEPS_PER_SECOND 25.0

// Frame times are expressed in frames at this rate
#define BASELINE_FPS 60.0

// Hexagons are drawn slightly smaller than their cell to leave a gap
#define HEX_DRAWN_RADIUS_FRACTION 0.9

// The coloured border takes this fraction of the drawn radius
#define HEX_BORDER_THICKNESS_FRACTION 0.2

// The number fits in a box of these fractions of the hexagon's inner radius
#define HEX_NUMBER_WIDTH_FRACTION 1.3
#define HEX_NUMBER_HEIGHT_FRACTION 0.9

#define HEX_FILL_COLOR 0x000000
#define SELECTED_HEX_NUMBER_COLOR 0x000000
#define EMPTY_CELL_BORDER_COLOR 0x303030
#define HOVERED_CELL_BORDER_COLOR 0xA0A0A0
#define HOVERED_CELL_FILL_COLOR 0x202020

#endif  // GAME_SRC_MAIN_GAME_CONSTANTS_H_
