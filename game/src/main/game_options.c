#include "game_options.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"

#define MIN_GROUP "--min-group="

void print_game_help(void) {
  puts("game options:");
  printf("\t" MIN_GROUP
         "N: how many equal hexagons must touch to merge, %d-%d "
         "(default: %d, more on bigger boards)\n\n",
         SMALLEST_MIN_GROUP, MAX_GROUP_SIZE, MIN_GROUP_SIZE);
}

// Reads the whole number after the option's name; false if it is not one
// or is out of range
static bool parse_min_group(const char* argument, int* min_group) {
  const char* text = argument + strlen(MIN_GROUP);
  char* end;
  errno = 0;
  int64_t number = strtol(text, &end, 10);
  if (errno == ERANGE || end == text || *end != '\0' ||
      number < SMALLEST_MIN_GROUP || number > MAX_GROUP_SIZE) {
    return false;
  }
  *min_group = (int)number;
  return true;
}

game_options_t take_game_options(int* argc, char* argv[]) {
  game_options_t options = {true, AUTOMATIC_MIN_GROUP};
  int kept = 0;
  for (int i = 0; i < *argc; ++i) {
    if (i == 0 || strncmp(argv[i], MIN_GROUP, strlen(MIN_GROUP)) != 0) {
      argv[kept++] = argv[i];
    } else if (!parse_min_group(argv[i], &options.min_group)) {
      fprintf(stderr, "Error: Invalid option '%s' (valid: %d-%d)\n", argv[i],
              SMALLEST_MIN_GROUP, MAX_GROUP_SIZE);
      fprintf(stderr, "Use --help to see all available options\n");
      options.valid = false;
    }
  }
  *argc = kept;
  return options;
}
