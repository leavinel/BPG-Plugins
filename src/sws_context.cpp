/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

extern "C" {
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
}

#include <exception>
#include "sws_context.hpp"
#include "av_util.hpp"
#include "threadpool.hpp"
#include "looptask.hpp"
#include "log.h"

using namespace std;
using namespace sws;
using namespace avutil;


Context::Context():
    w(0), h(0), algo(0),
    src ({NULL, NULL, AV_PIX_FMT_NONE, NULL, 1}),
    dst ({NULL, NULL, AV_PIX_FMT_NONE, NULL, 1}),
    brightness (0),
    contrast   (1 << 16),
    saturation (1 << 16)
{
    src.coeff =
    dst.coeff = sws_getCoefficients (SWS_CS_DEFAULT);
}


/** Convert quality number to algorithm flags */
uint32_t Context::quality2algo (int quality)
{
    /* Algorithm
     * @note SWS_FAST_BILINEAR is buggy if BPG_CS_RGB & limited_range
     */
    switch (quality)
    {
        case QUALITY_MIN:
            return SWS_POINT;
        case -1: // Default
        case 1:
            return SWS_BILINEAR;
        case 2 ... QUALITY_MAX-1:
            return SWS_BICUBIC;
        default:
            return SWS_BICUBIC | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND;
    }
}


void Context::Alloc (int w, int h, enum AVPixelFormat src_fmt, enum AVPixelFormat dst_fmt, int quality)
{
    this->w = w;
    this->h = h;
    src.fmt = src_fmt;
    src.desc = avutil::av_pix_fmt_desc_get (src_fmt);
    src.planes = avutil::av_pix_fmt_count_planes (src_fmt);
    dst.fmt = dst_fmt;
    dst.desc = avutil::av_pix_fmt_desc_get (dst_fmt);
    dst.planes = avutil::av_pix_fmt_count_planes (dst_fmt);
    algo = quality2algo (quality);
}


/**
 * Wrapper of sws_setColorspaceDetails
 * @note Requires colorspace code, not coefficients
 */
void Context::setColorSpace (
    int src_cs, int src_full_rng,
    int dst_cs, int dst_full_rng,
    int brightness, int contrast, int saturation
)
{
    src.coeff    = sws_getCoefficients (src_cs);
    src.full_rng = src_full_rng;
    dst.coeff    = sws_getCoefficients (dst_cs);
    dst.full_rng = dst_full_rng;
    this->brightness = brightness;
    this->contrast   = contrast;
    this->saturation = saturation;
}


int Context::scale (
    const uint8_t *srcSlice[],
    const int srcStride[],
    int srcSliceY, int srcSliceH,
    uint8_t *const dstSlice[], const int dstStride[]
)
{
    src.bufs = (void**)srcSlice;
    src.stride = srcStride;
    dst.bufs = (void**)dstSlice;
    dst.stride = dstStride;

    pSwsContext p (
        sws_getContext (w, h, src.fmt, w, h, dst.fmt, algo, NULL, NULL, NULL),
        sws_freeContext
    );

    if (!p)
        return -1;

    sws_setColorspaceDetails (
        p.get(),
        src.coeff, src.full_rng,
        dst.coeff, dst.full_rng,
        brightness, contrast, saturation
    );

    return sws_scale (p.get(), (const uint8_t**)src.bufs, src.stride, srcSliceY, srcSliceH, (uint8_t**)dst.bufs, dst.stride);
}



/**
 * Subtask for Context::scaleMT()
 */
class Context::convertTask: public LoopTask
{
private:
    Context &ctx;

    static bool isPLANAR (const attr &a);
    static bool isRGB (const attr &a);
    static void calcAddr (uint8_t *buf[4], const attr &a, uint32_t y);

public:
    convertTask (Context &ctx): ctx(ctx) {}
    virtual void loop (int begin, int end, int step) override;
};


bool Context::convertTask::isPLANAR (const attr &a)
{
    return !!(a.desc->flags & AV_PIX_FMT_FLAG_PLANAR);
}


bool Context::convertTask::isRGB (const attr &a)
{
    return !!(a.desc->flags & AV_PIX_FMT_FLAG_RGB);
}


/**
 * Perform Y offset on src/dst slice addresses
 */
void Context::convertTask::calcAddr (uint8_t *buf[4], const attr &a, uint32_t y)
{
    int i;

    /* Load original address */
    for (i = 0; i < a.planes; i++)
        buf[i] = (uint8_t*)a.bufs[i];

    if (y) // Need offset
    {
        if (isPLANAR(a) && !isRGB(a)) // Planar YUV
        {
            buf[0] += a.stride[0] * y;
            buf[1] += a.stride[1] * (y >> a.desc->log2_chroma_h);
            buf[2] += a.stride[2] * (y >> a.desc->log2_chroma_h);
            if (a.planes > 3) // A
                buf[3] += a.stride[3] * y;
        }
        else
        {
            for (i = 0; i < a.planes; i++)
                buf[i] += a.stride[i] * y;
        }
    }
}


void Context::convertTask::loop (int begin, int end, int step)
{
    const uint8_t *src[4];
    uint8_t *dst[4];
    int h;

    Logi ("%s: (%d, %d, %d)", __PRETTY_FUNCTION__, begin, end, step);
    calcAddr ((uint8_t**)src, ctx.src, begin);
    calcAddr (dst, ctx.dst, begin);

    if (ctx.src.planes == 0)
        return;
    for (int i = 0; i < ctx.src.planes; i++)
        Logi ("src plane[%d] %p -> %p", i, ctx.src.bufs[i], src[i]);
    for (int i = 0; i < ctx.dst.planes; i++)
        Logi ("dst plane[%d] %p -> %p", i, ctx.dst.bufs[i], dst[i]);

    h = end - begin;

    pSwsContext p (
        sws_getContext (ctx.w, h, ctx.src.fmt, ctx.w, h, ctx.dst.fmt, ctx.algo, NULL, NULL, NULL),
        sws_freeContext
    );

    if (!p)
        return;

    sws_setColorspaceDetails (
        p.get(),
        ctx.src.coeff, ctx.src.full_rng,
        ctx.dst.coeff, ctx.dst.full_rng,
        ctx.brightness, ctx.contrast, ctx.saturation
    );

    sws_scale (p.get(), src, ctx.src.stride, 0, h, dst, ctx.dst.stride);
}


/**
 * sws_scale() Multi-Thread version
 */
int Context::scaleMT (
    ThreadPool &pool,
    const uint8_t *srcSlice[],
    const int srcStride[],
    int srcSliceY, int srcSliceH,
    uint8_t *const dstSlice[], const int dstStride[]
)
{
    src.bufs = (void**)srcSlice;
    src.stride = srcStride;
    dst.bufs = (void**)dstSlice;
    dst.stride = dstStride;

    LoopTaskManager tasks (pool);
    tasks.SetLoopRange (0, h, 2);
    tasks.Dispatch<convertTask> (*this);
    return 0;
}
