#ifndef __JPEG_H
#define __JPEG_H
extern "C" {
#include <jpeglib.h>
}
#include <string>
#include <vector>
#include "buffer.h"
#include "utils.h"

bool	CVSimpleResizeJpeg(Buffer &buffer, int width, int height, int iRotate);
#endif
