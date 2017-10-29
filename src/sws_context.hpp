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
#include "libavutil/pixdesc.h"
#include "libswscale/swscale.h"
}

#include <memory>
#include <list>
#include "threadpool.hpp"


namespace sws {

typedef std::unique_ptr<struct SwsContext, void(*)(struct SwsContext*)> pSwsContext;


/**
 * Wrapper of SwsContext with Multi-Thread capability
 */
class Context
{
private:
    class convertTask;

    uint32_t w, h;
    uint32_t algo;

    /** Source / destination attributes */
    struct attr {
        void **bufs;
        const int *stride;
        enum AVPixelFormat fmt;
        const AVPixFmtDescriptor *desc;
        uint8_t planes;
        const int *coeff;
        uint8_t full_rng;
    } src, dst;

    int brightness;
    int contrast;
    int saturation;

    static uint32_t quality2algo (int quality);

public:
    enum {
        QUALITY_MIN = 0,
        QUALITY_MAX = 9
    };

    Context();

    void Alloc (int w, int h, enum AVPixelFormat src_fmt, enum AVPixelFormat dst_fmt, int quality = QUALITY_MAX);

    void setColorSpace (
        int src_cs, int src_full_rng,
        int dst_cs, int dst_full_rng,
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

    int scaleMT (
        ThreadPool &pool,
        const uint8_t *srcSlice[],
        const int srcStride[],
        int srcSliceY, int srcSliceH,
        uint8_t *const dst[], const int dstStride[]
    );
};

}

#endif /* _SWS_CONTEXT_HPP_ */
