#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "command_line.h"
#include "game.h"
#include "game_options.h"
#include "graphics.h"
#include "logger.h"
#include "stage.h"

static bool run_game(const game_ptr game) {
  stage_ptr stage = create_playing_stage_instance();
  if (!stage) {
    LOG_ERROR("Unable to allocate game stages");
    return false;
  }

  stage->init(stage, game);
  bool succeeded = stage->state != NULL;
  if (succeeded) {
    stage->update(stage);
  } else {
    LOG_ERROR("Unable to allocate game stage state");
  }

  destroy_stage(stage);
  return succeeded;
}

int main(int argc, char* argv[]) {
  srand(time(NULL));

  game_options_t game_options = take_game_options(&argc, argv);
  if (!game_options.valid) {
    return EXIT_FAILURE;
  }

  command_line_options_t command_line_options =
      parse_command_line_options(argc, argv);

  if (command_line_options.help) {
    print_help();
    print_game_help();
    return 0;
  }

  if (command_line_options.graphics_info) {
    print_graphics_info();
    return 0;
  }

  game_settings_t game_settings = init_game_settings(
      command_line_options.show_fps, command_line_options.vsync,
      command_line_options.display, command_line_options.display_mode,
      command_line_options.window_mode, command_line_options.fps,
      game_options.min_group);

  game_t game = init_game(game_settings);

  bool succeeded = run_game(&game);

  terminate_game(&game);

  return succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
