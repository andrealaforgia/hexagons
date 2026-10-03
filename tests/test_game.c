#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "game_settings.h"
#include "playing_stage.h"
#include "stage.h"
#include "test_allocator.h"

static game_t test_game(void) {
  game_t game = {0};
  game.settings = init_game_settings(false, false, 0, 0, WINDOWED, 60);
  game.graphics_context.screen_width = 1440;
  game.graphics_context.screen_height = 900;
  game.graphics_context.screen_center = point(720, 450);
  return game;
}

static void stages_release_every_allocation(void) {
  game_t game = test_game();
  for (int replay = 0; replay < 10; ++replay) {
    playing_stage_state_ptr state = create_playing_stage(&game);
    assert(state);
    destroy_playing_stage(state);
    assert(outstanding_allocations() == 0);
  }
}

static void stage_creation_cleans_up_on_failure(void) {
  game_t game = test_game();
  bool succeeded = false;
  for (int allocation = 0; allocation < 64; ++allocation) {
    fail_allocation_after(allocation);
    playing_stage_state_ptr state = create_playing_stage(&game);
    if (state) {
      destroy_playing_stage(state);
      assert(outstanding_allocations() == 0);
      succeeded = true;
      break;
    }
    assert(outstanding_allocations() == 0);
  }
  assert(succeeded);
  fail_allocation_after(-1);
}

static void stage_factories_handle_allocation_failure(void) {
  fail_allocation_after(0);
  assert(create_playing_stage_instance() == NULL);
  fail_allocation_after(-1);
  stage_ptr stage = create_playing_stage_instance();
  assert(stage);
  destroy_stage(stage);
  assert(outstanding_allocations() == 0);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  if (!strcmp(argv[1], "lifecycle"))
    stages_release_every_allocation();
  else if (!strcmp(argv[1], "allocation"))
    stage_creation_cleans_up_on_failure();
  else if (!strcmp(argv[1], "factories"))
    stage_factories_handle_allocation_failure();
  else
    return 1;
  return 0;
}
