#include "hex_colors.h"

#include <math.h>

#include "color.h"

// The numbers are the powers of two from 1 to 1024
#define NUMBER_COUNT 11
// Hues are spread evenly round the colour wheel, and handed out this many
// places apart so that a number and its double look nothing alike
#define HUE_STRIDE 4
#define BORDER_SATURATION 0.75
// How far the selected fill moves from the border colour towards white
#define SELECTED_FILL_WHITENESS 0.55

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

// Position of a number in the sequence 1, 2, 4, 8...
static int number_index(int value) {
  int index = 0;
  while (value > 1) {
    value /= 2;
    ++index;
  }
  return index;
}

color_t hex_border_color(int value) {
  int place = number_index(value) * HUE_STRIDE % NUMBER_COUNT;
  return bright_color(place * 360.0 / NUMBER_COUNT, BORDER_SATURATION);
}

static int lighten(int channel) {
  return channel + (int)lround((255 - channel) * SELECTED_FILL_WHITENESS);
}

color_t hex_selected_fill_color(int value) {
  color_t border = hex_border_color(value);
  return COLOR(lighten(R(border)), lighten(G(border)), lighten(B(border)));
}
