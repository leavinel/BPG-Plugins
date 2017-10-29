#include <string.h>
#include <exception>

#include "frame.hpp"
#include "bpg_common.hpp"

using namespace std;
using namespace bpg;

/**
 * Allocate buffer by bits-per-pixel
 */
void Frame::AllocByBpp (int w, int h, int bpp)
{
    switch (bpp)
    {
        case 8:  fmt = AV_PIX_FMT_GRAY8; break;
        case 24: fmt = AV_PIX_FMT_RGB24; break;
        case 32: fmt = AV_PIX_FMT_RGBA; break;
        default: throw runtime_error ("invalid format");
    }

    AllocByFormat (w, h, fmt);
}


void Frame::AllocByFormat (int w, int h, enum AVPixelFormat fmt)
{
    size_t bufsz;

    SetFormat (w, h, fmt);
    bufsz = stride * h;
    buf = unique_ptr<uint8_t[]> (new uint8_t[bufsz]);
    ptr = buf.get();
}


/**
 * Set frame format (stride will be calculated)
 */
void FrameDesc::SetFormat (int w, int h, enum AVPixelFormat fmt)
{
    this->w = w;
    this->h = h;
    this->fmt = fmt;

    switch (fmt)
    {
        case AV_PIX_FMT_GRAY8: stride = w;     break;
        case AV_PIX_FMT_RGB24:
        case AV_PIX_FMT_BGR24: stride = w * 3; break;
        case AV_PIX_FMT_RGBA:
        case AV_PIX_FMT_BGRA:  stride = w * 4; break;
        default: throw runtime_error ("invalid format");
    }
}


void FrameDesc::GetLine (int y, void *dst) const
{
    memcpy (dst, (const uint8_t*)ptr + y * stride, stride);
}


void FrameDesc::SetLine (int y, const void *src)
{
    uint8_t *dst = (uint8_t*)ptr + y * stride;
    memcpy (dst, src, stride);
}

