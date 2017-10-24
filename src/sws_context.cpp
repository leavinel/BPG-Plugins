/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

extern "C" {
#include "libavutil/common.h"
#include "libswscale/swscale.h"
}

#include "sws_context.hpp"

using namespace sws;


sws::Context::Context(): p(NULL)
{
}

sws::Context::~Context()
{
    sws_freeContext (p);
}


void Context::get (int w, int h, enum AVPixelFormat src_fmt, enum AVPixelFormat dst_fmt, int algo)
{
    p = sws_getContext (w, h, src_fmt, w, h, dst_fmt, algo, NULL, NULL, NULL);
}


void Context::setColorSpace (
    const int src_coeff[], int src_full_rng,
    const int dst_coeff[], int dst_full_rng,
    int brightness, int contrast, int saturation
)
{
    sws_setColorspaceDetails (
        p,
        src_coeff, src_full_rng,
        dst_coeff, dst_full_rng,
        brightness, contrast, saturation
    );
}


int Context::scale (
    const uint8_t *srcSlice[],
    const int srcStride[],
    int srcSliceY, int srcSliceH,
    uint8_t *const dst[], const int dstStride[]
)
{
    if (p)
        return sws_scale (p, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
    else
        return -1;
}
