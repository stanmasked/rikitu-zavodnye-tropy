#include "rikitu_background_blit.h"
#include "rikitu_parallax.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

/* Bound the tested scene/texture space as a guard against overflow or typo. */
static int valid_extent(int width,int height,int stride,int max_width){
  return width>0 && height>0 && width<=max_width && height<=2160 &&
         stride>=width && stride<=32768;
}

static int fits_rows(size_t capacity,int width,int height,int stride){
  const size_t rows=(size_t)(height-1);
  if(rows>SIZE_MAX/(size_t)stride)return 0;
  return rows*(size_t)stride+(size_t)width<=capacity;
}

/* Reject aliasing even when pointers differ inside the same allocation.
 * Treat each padded row span as occupied; conservative rejection is safer
 * than memcpy with overlapping source/destination. uintptr_t guards avoid
 * overflowing while comparing address intervals from separate allocations.
 */
static int disjoint_pixel_spans(const void *a,size_t a_pixels,
                                const void *b,size_t b_pixels){
  const uintptr_t a_start=(uintptr_t)a;
  const uintptr_t b_start=(uintptr_t)b;
  uintptr_t a_bytes,b_bytes;
  if(a_pixels>UINTPTR_MAX/sizeof(uint32_t) ||
     b_pixels>UINTPTR_MAX/sizeof(uint32_t))return 0;
  a_bytes=(uintptr_t)a_pixels*sizeof(uint32_t);
  b_bytes=(uintptr_t)b_pixels*sizeof(uint32_t);
  if(a_bytes>UINTPTR_MAX-a_start || b_bytes>UINTPTR_MAX-b_start)return 0;
  return a_start+a_bytes<=b_start || b_start+b_bytes<=a_start;
}

int rikitu_background_blit_rgba8888(
    uint32_t *dst, size_t dst_pixel_count,
    int viewport_width, int viewport_height, int dst_stride_pixels,
    const RikituBackgroundImage *source, double camera_world,
    double world_width, int *out_crop_x){
  int x, row;
  if(!dst||!source||!source->pixels||dst==source->pixels||
     !valid_extent(viewport_width,viewport_height,dst_stride_pixels,3840)||
     !valid_extent(source->width,source->height,source->stride_pixels,16384)||
     source->height!=viewport_height || source->width<viewport_width ||
     !isfinite(camera_world) || !isfinite(world_width) ||
     !fits_rows(dst_pixel_count,viewport_width,viewport_height,
                dst_stride_pixels) ||
     !fits_rows(source->pixel_count,source->width,source->height,
                source->stride_pixels))
    return 0;
  if(!disjoint_pixel_spans(
       dst,(size_t)(viewport_height-1)*(size_t)dst_stride_pixels+
           (size_t)viewport_width,
       source->pixels,(size_t)(source->height-1)*
           (size_t)source->stride_pixels+(size_t)source->width))return 0;
  if(!rikitu_parallax_crop_x(camera_world,world_width,viewport_width,
                             source->width,&x))return 0;
  if(x<0 || x>source->width-viewport_width)return 0;
  for(row=0;row<viewport_height;row++){
    const size_t dest_off=(size_t)row*(size_t)dst_stride_pixels;
    const size_t src_off=(size_t)row*(size_t)source->stride_pixels+(size_t)x;
    memcpy(dst+dest_off,source->pixels+src_off,
           (size_t)viewport_width*sizeof(uint32_t));
  }
  if(out_crop_x)*out_crop_x=x;
  return 1;
}
