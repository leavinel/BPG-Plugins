/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _SRC_FRAME_HPP_
#define _SRC_FRAME_HPP_

#include <stdint.h>

extern "C" {
#include "libavutil/pixfmt.h"
}

#include <memory>

namespace bpg {


class ImageInfo;

/**
 * Image frame descriptor
 */
class FrameDesc
{
public:
    uint32_t w, h;
    enum AVPixelFormat fmt;
    void *ptr;
    uint32_t stride;

    FrameDesc(): w(0), h(0), fmt(AV_PIX_FMT_NONE), ptr(NULL), stride(0) {}

    void SetFormat (int w, int h, enum AVPixelFormat fmt);

    operator bool() const { return (bool)ptr; }

    void GetLine (int y, void *dst) const;
    void SetLine (int y, const void *src);
};


/**
 * Image frame buffer with auto-allocated buffer
 */
class Frame: public FrameDesc
{
private:
    std::unique_ptr<uint8_t[]> buf;

public:
    Frame(){}
    void AllocByBpp (int w, int h, int bpp);
    void AllocByFormat (int w, int h, enum AVPixelFormat fmt);
};

}

#endif /* _SRC_FRAME_HPP_ */
