/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _SWS_CONTEXT_HPP_
#define _SWS_CONTEXT_HPP_

extern "C" {
#include "libavutil/pixfmt.h"
}

struct SwsContext;

namespace sws {

class Context {
private:
    struct SwsContext *p;

public:
    Context();
    ~Context();

    void get (int w, int h, enum AVPixelFormat src_fmt, enum AVPixelFormat dst_fmt, int algo);

    void setColorSpace (
        const int src_coeff[], int src_full_rng,
        const int dst_coeff[], int dst_full_rng,
        int brightness = 0,
        int contrast   = 1 << 16,
        int saturation = 1 << 16
    );

    int scale (
        const uint8_t *srcSlice[],
        const int srcStride[],
        int srcSliceY, int srcSliceH,
        uint8_t *const dst[], const int dstStride[]
    );
};

}

#endif /* _SWS_CONTEXT_HPP_ */
