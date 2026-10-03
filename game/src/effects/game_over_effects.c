#include "game_over_effects.h"

#include <math.h>
#include <stdbool.h>

#include "geometry.h"

#define FLASH_PERIOD_SECONDS 0.7
#define FLASH_SHOWN_FRACTION 0.65
#define BOB_PERIOD_SECONDS 2.4

bool is_game_over_title_shown(double seconds) {
  return fmod(seconds, FLASH_PERIOD_SECONDS) <
         FLASH_PERIOD_SECONDS * FLASH_SHOWN_FRACTION;
}

double game_over_prompt_offset(double seconds, double amplitude) {
  return amplitude * sin(2 * M_PI * seconds / BOB_PERIOD_SECONDS);
}
