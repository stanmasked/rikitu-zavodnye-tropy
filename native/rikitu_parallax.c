#include "rikitu_parallax.h"

#include <math.h>

int rikitu_parallax_crop_x(double camera_world, double world_width,
                          int viewport_width, int panorama_width,
                          int *out_source_x) {
  double world_extent;
  double image_extent;
  double position;

  if(!out_source_x || !isfinite(camera_world) || !isfinite(world_width) ||
     viewport_width <= 0 || panorama_width < viewport_width ||
     world_width < (double)viewport_width) {
    return 0;
  }

  world_extent = world_width - (double)viewport_width;
  image_extent = (double)panorama_width - (double)viewport_width;

  /* Zero-camera-range case: no scrolling; source crop begins at x=0. */
  if(world_extent <= 0.0) {
    *out_source_x = 0;
    return 1;
  }

  if(camera_world < 0.0) camera_world = 0.0;
  if(camera_world > world_extent) camera_world = world_extent;

  position = camera_world * (image_extent / world_extent);
  if(position < 0.0) position = 0.0;
  if(position > image_extent) position = image_extent;

  *out_source_x = (int)floor(position + 0.5);
  return 1;
}
