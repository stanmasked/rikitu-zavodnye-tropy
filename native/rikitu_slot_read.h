#ifndef RIKITU_SLOT_READ_H
#define RIKITU_SLOT_READ_H
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
/* File-level result before the format-specific CRC/codec check.
 * ERROR means inspection failed and callers must fail closed.
 * INVALID means absent or readable but not exactly one fixed-size record.
 * OK means exactly size bytes were read and read/close both succeeded.
 */
enum {
  RIKITU_SLOT_READ_ERROR = -1,
  RIKITU_SLOT_READ_INVALID = 0,
  RIKITU_SLOT_READ_OK = 1
};
static int rikitu_read_fixed_slot(const char *path, unsigned char *out,
                                  size_t size) {
  FILE *f;
  size_t bytes;
  int extra, read_error, close_error;
  if (!path || !out || size == 0)
    return RIKITU_SLOT_READ_ERROR;
  errno = 0;
  f = fopen(path, "rb");
  if (!f)
    return errno == ENOENT ? RIKITU_SLOT_READ_INVALID
                           : RIKITU_SLOT_READ_ERROR;
  bytes = fread(out, 1, size, f);
  read_error = ferror(f);
  extra = fgetc(f);
  if (ferror(f))
    read_error = 1;
  close_error = fclose(f);
  if (read_error || close_error != 0)
    return RIKITU_SLOT_READ_ERROR;
  if (bytes != size || extra != EOF)
    return RIKITU_SLOT_READ_INVALID;
  return RIKITU_SLOT_READ_OK;
}
#endif
