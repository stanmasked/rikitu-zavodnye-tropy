#ifndef RIKITU_BACKGROUND_BLIT_H
#define RIKITU_BACKGROUND_BLIT_H

/* Asset-independent RGBA8888 panorama copier. Source pixels belong to caller.
 * Byte order matches the framebuffer: 0xAABBGGRR (libnx RGBA8).
 * This module intentionally has no file IO, decoding, approved art or game state.
 */
#include <stddef.h>
#include <stdint.h>

typedef struct RikituBackgroundImage {
  const uint32_t *pixels;
  size_t pixel_count;
  int width, height, stride_pixels;
} RikituBackgroundImage;

/* Copy the camera-selected, unscaled source slice to a framebuffer.
 * Contract: source/destination buffers must not overlap (including different
 * pointers into the same allocation); the blit rejects overlapping spans.
 * The caller draws
 * foreground/gameplay/HUD afterwards. Source height equals viewport height.
 * For invalid arguments return 0 without changing destination/out_crop_x.
 * This is NOT wired into the game renderer until art is approved.
 */
int rikitu_background_blit_rgba8888(
    uint32_t *dst, size_t dst_pixel_count,
    int viewport_width, int viewport_height, int dst_stride_pixels,
    const RikituBackgroundImage *source, double camera_world,
    double world_width, int *out_crop_x);

#endif
