#include <curves.h>
#include <color.h>
# include "../../dng_sdk_1_7/dng_sdk/source/dng_pixel_buffer.h"

namespace mini_rt{

void firstAnalysis(dng_pixel_buffer& pixel_buf, int width, int hight, LUTu & histogram);
int do_contrast(dng_pixel_buffer& pixel_buf, size_t height, size_t width, double contrast, LUTf& contrast_curve);

}
