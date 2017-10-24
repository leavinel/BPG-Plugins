/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */


extern "C" {
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "libswscale/swscale.h"
}

#include <string>
#include <vector>
#include "bpg_common.hpp"
#include "looptask.hpp"
#include "benchmark.hpp"


#define GETBYTE(val,n)      (((val) >> ((n) * 8)) & 0xFF)

using namespace std;
using namespace bpg;


/**
 * Load a BPG image info from buffer
 */
void ImageInfo::LoadFromBuffer (const void *buf, size_t len)
{
    FAIL_THROW (bpg_decoder_get_info_from_buf (this, NULL, (uint8_t*)buf, len));
}


uint8_t ImageInfo::GetBpp() const
{
    if (format == BPG_FORMAT_GRAY)
        return 8;

    if (has_alpha)
        return 32;

    return 24;
}


/**
 * Check if header magic number matches
 */
bool ImageInfo::CheckHeader (const void *buf, size_t len)
{
    static const uint8_t magic[HEADER_MAGIC_SIZE] = {
        GETBYTE(BPG_HEADER_MAGIC,3),
        GETBYTE(BPG_HEADER_MAGIC,2),
        GETBYTE(BPG_HEADER_MAGIC,1),
        GETBYTE(BPG_HEADER_MAGIC,0),
    };

    if (len < sizeof(magic))
        return false;

    return 0 == memcmp (&magic, buf, HEADER_MAGIC_SIZE);
}


/**
 * Allocate buffer
 */
void Frame::Alloc (const ImageInfo &info)
{
    size_t bufsz;
    int bpp;

    bpp = info.GetBpp();
    stride = info.width * bpp / 8;
    bufsz = stride * info.height;
    buf = unique_ptr<uint8_t[]> (new uint8_t[bufsz]);

    bReady = true;
}


void Frame::GetLine (int y, void *dst)
{
    memcpy (dst, buf.get() + y * stride, stride);
}


Decoder::Decoder(): ctx(bpg_decoder_open(), bpg_decoder_close)
{
    if (!ctx)
        throw runtime_error ("bpg_decoder_open");
}


void Decoder::DecodeBuffer (const void *buf, size_t len, uint8_t opts)
{
    Benchmark bm ("BPG decode");
    BPGDecoderContext *ctx2 = ctx.get();

    if (opts & OPT_HEADER_ONLY)
    {
        FAIL_THROW (bpg_decoder_decode_header_only (ctx2, (uint8_t*)buf, len));
    }
    else
    {
        FAIL_THROW (bpg_decoder_decode (ctx2, (uint8_t*)buf, len));
    }

    FAIL_THROW (bpg_decoder_get_info (ctx2, &info));
}


void Decoder::DecodeFile (const char s_file[], uint8_t opts)
{
    FILE *fp = fopen (s_file, "rb");

    if (!fp)
        throw runtime_error (std::string("Cannot open file: ") + s_file);

    fseek (fp, 0, SEEK_END);
    size_t fsize = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    vector<uint8_t> buf (fsize);

    if (fsize != fread (&buf[0], 1, fsize, fp))
        throw runtime_error ("Failed to read file");

    fclose (fp);
    DecodeBuffer (&buf[0], fsize, opts);
}


/**
 * Parameters for doConvert()
 */
struct Decoder::convertParam
{
    struct {
        enum AVPixelFormat fmt;
        const uint8_t *buf[4];
        int stride[4];
        const int *coeff;
        uint8_t fullRng;
    } src;

    struct {
        enum AVPixelFormat fmt;
        uint8_t *buf[1];
        int stride[1];
        const int *coeff;
        uint8_t fullRng;
    } dst;

    int brightness;
    int contrast;
    int algo;
};


/**
 * Prepare convertParam for doConvert()
 */
int Decoder::prepareConvert (
    convertParam &par,
    enum AVPixelFormat dst_fmt,
    void *dst,
    int dst_stride,
    int quality
)
{
    enum AVPixelFormat src_fmt;
    const int *src_coeff;

    /* Find source format */
    switch (info.format)
    {
    case BPG_FORMAT_GRAY:
        src_fmt = AV_PIX_FMT_GRAY16BE;
        break;
    case BPG_FORMAT_420:
    case BPG_FORMAT_420_VIDEO:
        if (info.has_alpha)
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUVA420P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUVA420P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUVA420P10LE; break;
            default: return -1;
            }
        }
        else
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUV420P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUV420P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUV420P10LE; break;
            case 12: src_fmt = AV_PIX_FMT_YUV420P12LE; break;
            case 14: src_fmt = AV_PIX_FMT_YUV420P14LE; break;
            default: return -1;
            }
        }
        break;
    case BPG_FORMAT_422:
    case BPG_FORMAT_422_VIDEO:
        if (info.has_alpha)
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUVA422P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUVA422P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUVA422P10LE; break;
            default: return -1;
            }
        }
        else
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUV422P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUV422P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUV422P10LE; break;
            case 12: src_fmt = AV_PIX_FMT_YUV422P12LE; break;
            case 14: src_fmt = AV_PIX_FMT_YUV422P14LE; break;
            default: return -1;
            }
        }
        break;
    case BPG_FORMAT_444:
        if (info.color_space == BPG_CS_RGB)
        {
            if (info.has_alpha)
                src_fmt = AV_PIX_FMT_GBRAP16BE;
            else
                src_fmt = AV_PIX_FMT_GBRP16BE;
        }
        else if (info.has_alpha)
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUVA444P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUVA444P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUVA444P10LE; break;
            default: return -1;
            }
        }
        else
        {
            switch (info.bit_depth)
            {
            case 8:  src_fmt = AV_PIX_FMT_YUV444P16BE; break;
            case 9:  src_fmt = AV_PIX_FMT_YUV444P9LE;  break;
            case 10: src_fmt = AV_PIX_FMT_YUV444P10LE; break;
            case 12: src_fmt = AV_PIX_FMT_YUV444P12LE; break;
            case 14: src_fmt = AV_PIX_FMT_YUV444P14LE; break;
            default: return -1;
            }
        }
        break;
    default:
        return -1;
    }

    par.src.fmt = src_fmt;
    par.dst.fmt = dst_fmt;

    /* Framebuffer */
    for (int i = 0; i < 4; i++)
        par.src.buf[i] = bpg_decoder_get_data (ctx.get(), par.src.stride+i, i);

    par.dst.buf[0] = (uint8_t*)dst;
    par.dst.stride[0] = dst_stride;

    /* White-black range */
    par.src.fullRng = 1;
    par.dst.fullRng = 1;
    par.brightness = 0;
    par.contrast = 1 << 16;

    if (info.limited_range)
    {
        if (info.format == BPG_FORMAT_GRAY)
        {
            /* Not supported */
        }
        else if (info.color_space == BPG_CS_RGB)
        {
            /* dstRange has no effect on RGB -> RGB conversion, so here we use
             * brightness / contrast to restore (16,235)->(0,255)
             */
            par.brightness = -(16 << 8) * 219 / 255;
            par.contrast   = (1 << 16) * 255 / 219;
        }
        else
        {
            /* YUV -> RGB */
            par.src.fullRng = 0;
        }
    }

    /* Algorithm
     * @note SWS_FAST_BILINEAR is buggy if BPG_CS_RGB & limited_range
     */
    switch (quality)
    {
    case 0:
        par.algo = SWS_POINT;
        break;
    case 1:
        par.algo = SWS_BILINEAR;
        break;
    case 2 ... 8:
        par.algo = SWS_BICUBIC;
        break;
    case 9:
    default:
        par.algo = SWS_BICUBIC | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND;
        break;
    }

    /* Specify color space */
    switch (info.color_space)
    {
    case BPG_CS_YCbCr:
        src_coeff = sws_getCoefficients (SWS_CS_DEFAULT);
        break;
    case BPG_CS_YCbCr_BT709:
        src_coeff = sws_getCoefficients (SWS_CS_ITU709);
        break;
    case BPG_CS_YCbCr_BT2020:
        src_coeff = sws_getCoefficients (SWS_CS_BT2020);
        break;
    default:
        src_coeff = sws_getCoefficients (SWS_CS_DEFAULT);
        break;
    }

    par.src.coeff = src_coeff;
    par.dst.coeff = sws_getCoefficients (SWS_CS_DEFAULT);
    return 0;
}


/**
 * Perform partial/full conversion using sws_scale()
 */
int Decoder::doConvert (
    sws::Context &swsCtx,
    const convertParam &par,
    int cvt_y,      ///< Starting Y index to convert
    int cvt_h       ///< Height to convert
)
{
    const uint8_t *src[4];
    uint8_t *dst[1];

    swsCtx.get (info.width, cvt_h, par.src.fmt, par.dst.fmt, par.algo);
    swsCtx.setColorSpace (
        par.src.coeff, par.src.fullRng,
        par.dst.coeff, par.dst.fullRng,
        par.brightness, par.contrast
    );

    for (int i = 0; i < 4; i++)
        src[i] = par.src.buf[i];

    dst[0] = par.dst.buf[0];

    if (cvt_y)
    {
        /* Offset source image address */
        src[0] += par.src.stride[0] * cvt_y; // Y plane
        src[3] += par.src.stride[3] * cvt_y; // A plane

        switch (info.format)
        {
        case BPG_FORMAT_420:
        case BPG_FORMAT_420_VIDEO:
            src[1] += par.src.stride[1] * cvt_y / 2;
            src[2] += par.src.stride[2] * cvt_y / 2;
            break;
        case BPG_FORMAT_422:
        case BPG_FORMAT_422_VIDEO:
        case BPG_FORMAT_444:
            src[1] += par.src.stride[1] * cvt_y;
            src[2] += par.src.stride[2] * cvt_y;
            break;
        }

        /* Offset destination image addr */
        dst[0] += par.dst.stride[0] * cvt_y;
    }

    return swsCtx.scale (src, par.src.stride, 0, cvt_h, dst, par.dst.stride);
}


/**
 * Convert decoded frame to specified format
 */
int Decoder::Convert (
    enum AVPixelFormat dst_fmt,
    void *dst,
    int dst_stride,
    int quality         ///< Conversion quality (0: lowest, 9: highest)
)
{
    Benchmark bm ("BPG convert");
    sws::Context swsCtx;
    convertParam par;
    int ret;

    ret = prepareConvert (par, dst_fmt, dst, dst_stride, quality);
    if (ret < 0)
        return ret;
    return doConvert (swsCtx, par, 0, info.height);
}


/**
 * Subtask for Decoder::ConvertMT()
 */
class Decoder::convertTask: public LoopTask
{
private:
    sws::Context ctx;
    Decoder &dec;
    const convertParam &p;

public:
    convertTask (Decoder &dec, const convertParam &param):
        dec(dec), p(param) {}

    virtual void loop (int begin, int end, int step) override {
        dec.doConvert (ctx, p, begin, end - begin);
    }
};


/**
 * Perform Convert() in multi-thread, should be faster than Convert()
 */
int Decoder::ConvertMT (
    ThreadPool &pool,
    enum AVPixelFormat dst_fmt,
    void *dst,
    int dst_stride,
    int quality     ///< Conversion quality (0: lowest, 1: highest)
)
{
    Benchmark bm ("BPG convertMT");
    int ret;
    convertParam param;
    ret = prepareConvert (param, dst_fmt, dst, dst_stride, quality);
    if (ret < 0)
        return ret;

    /* YUV420 YUV422 require starting Y to be multiple of 2 */
    LoopTaskManager tasks (pool);
    tasks.SetLoopRange (0, info.height, 2);
    tasks.Dispatch<convertTask> (*this, param);
    return 0;
}


/**
 * Convert to a frame buffer
 */
int Decoder::ConvertToFrame (Frame &frame, ThreadPool &pool, int quality)
{
    enum AVPixelFormat dst_fmt;
    void *dst;
    int dst_stride;

    frame.Alloc (info);
    dst = frame.GetBuffer();
    dst_stride = frame.GetStride();

    switch (info.GetBpp())
    {
    case 8:  dst_fmt = AV_PIX_FMT_GRAY8; break;
    case 24: dst_fmt = AV_PIX_FMT_RGB24; break;
    case 32: dst_fmt = AV_PIX_FMT_RGBA;  break;
    default: dst_fmt = AV_PIX_FMT_NONE;  break;
    }

    if (dst_fmt == AV_PIX_FMT_NONE)
        return 0;

    //return Convert (dst_fmt, dst, dst_stride, b_hq);
    return ConvertMT (pool, dst_fmt, dst, dst_stride, quality);
}


void ImageInfo::GetFormatDetail (char buf[], size_t sz) const
{
    static const char *s_fmt[] = {
        "Grayscale",
        "4:2:0",
        "4:2:2",
        "4:4:4",
        "4:2:0V",
        "4:2:2V",
    };
    static const char *s_cs[] = {
        "YCbCr",
        "RGB",
        "YCgCo",
        "YCbCr(Bt.709)",
        "YCbCr(Bt.2020)"
    };

    snprintf (buf, sz, "BPG %s %s", s_fmt[format], s_cs[color_space]);
}


void ImageInfo::GetFormatDetail (string &s) const
{
    char buf[128];
    GetFormatDetail (buf, sizeof(buf));
    s = buf;
}
