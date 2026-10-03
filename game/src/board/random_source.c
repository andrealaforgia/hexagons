#include "random_source.h"

#include <stdint.h>

random_source_t create_random_source(uint32_t seed) {
  random_source_t random = {seed};
  return random;
}

// SplitMix32: any seed is valid and neighbouring seeds diverge at once
static uint32_t next(random_source_t* random) {
  uint32_t z = (random->state += 0x9E3779B9u);
  z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
  z = (z ^ (z >> 13)) * 0xC2B2AE35u;
  return z ^ (z >> 16);
}

int random_below(random_source_t* random, int bound) {
  if (bound <= 0) {
    return 0;
  }
  return (int)(next(random) % (uint32_t)bound);
}
