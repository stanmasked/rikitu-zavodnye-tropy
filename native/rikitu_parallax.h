#ifndef RIKITU_PARALLAX_H
#define RIKITU_PARALLAX_H

/* Geometry-only source crop for a noninteractive parallax background.
 * Does not read assets, mutate a scene, or change collision geometry.
 * All pixel dimensions refer to full-resolution image pixels.
 */
int rikitu_parallax_crop_x(double camera_world, double world_width,
                          int viewport_width, int panorama_width,
                          int *out_source_x);

#endif
