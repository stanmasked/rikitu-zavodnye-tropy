#include "rikitu_parallax.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_map01_full_scroll(void) {
  const double world_width = 7900.0;
  const int viewport_width = 1280;
  const int source_width = 5120;
  const double max_camera = world_width - (double)viewport_width;
  int previous = -1;
  int crop = -1;
  int i;

  for(i = 0; i <= 1000; ++i) {
    const double camera = max_camera * (double)i / 1000.0;
    assert(rikitu_parallax_crop_x(camera, world_width,
                                 viewport_width, source_width, &crop));
    assert(crop >= previous);
    assert(crop >= 0 && crop + viewport_width <= source_width);
    previous = crop;
  }
  assert(crop == source_width - viewport_width);
  assert(rikitu_parallax_crop_x(max_camera / 2.0, world_width,
                               viewport_width, source_width, &crop));
  assert(crop == 1920);
  assert(rikitu_parallax_crop_x(-1000.0, world_width,
                               viewport_width, source_width, &crop));
  assert(crop == 0);
  assert(rikitu_parallax_crop_x(100000.0, world_width,
                               viewport_width, source_width, &crop));
  assert(crop == 3840);
}

static void test_invalid_inputs_do_not_modify_output(void) {
  int crop = 27;
  assert(!rikitu_parallax_crop_x(NAN, 7900, 1280, 5120, &crop));
  assert(!rikitu_parallax_crop_x(INFINITY, 7900, 1280, 5120, &crop));
  assert(!rikitu_parallax_crop_x(0, NAN, 1280, 5120, &crop));
  assert(!rikitu_parallax_crop_x(0, INFINITY, 1280, 5120, &crop));
  assert(!rikitu_parallax_crop_x(0, 7900, 0, 5120, &crop));
  assert(!rikitu_parallax_crop_x(0, 7900, 1280, 1279, &crop));
  assert(!rikitu_parallax_crop_x(0, 1279, 1280, 5120, &crop));
  assert(!rikitu_parallax_crop_x(0, 7900, 1280, 5120, NULL));
  assert(crop == 27);
}

static void test_flat_source_and_static_world(void) {
  int crop = -1;
  assert(rikitu_parallax_crop_x(200, 1280, 1280, 5120, &crop));
  assert(crop == 0);
  assert(rikitu_parallax_crop_x(3900, 7900, 1280, 1280, &crop));
  assert(crop == 0);
}

int main(void) {
  test_map01_full_scroll();
  test_invalid_inputs_do_not_modify_output();
  test_flat_source_and_static_world();
  puts("PASS GFX-01 native safe panorama crop: monotonic, bounded and independent of gameplay");
  return 0;
}
