#include "hex_colors.h"

#include <math.h>

#include "color.h"

#define GOLDEN_ANGLE_DEGREES 137.50776405003785
#define BORDER_SATURATION 0.75

// Fully bright colour of the given hue (degrees) and saturation
static color_t bright_color(double hue, double saturation) {
  double sector = hue / 60;
  double ramp = 1 - fabs(fmod(sector, 2) - 1);
  double high = 1, mid = 1 - saturation * (1 - ramp), low = 1 - saturation;
  double channels[6][3] = {{high, mid, low}, {mid, high, low},
                           {low, high, mid}, {low, mid, high},
                           {mid, low, high}, {high, low, mid}};
  const double* rgb = channels[(int)sector % 6];
  return COLOR((int)lround(rgb[0] * 255), (int)lround(rgb[1] * 255),
               (int)lround(rgb[2] * 255));
}

color_t hex_border_color(int value) {
  // Stepping the hue by the golden angle keeps consecutive numbers, and any
  // small set of numbers, far apart on the colour wheel
  return bright_color(fmod(value * GOLDEN_ANGLE_DEGREES, 360),
                      BORDER_SATURATION);
}
