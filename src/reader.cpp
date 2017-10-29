/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */


#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" {
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


Decoder::Decoder():
    ctx (bpg_decoder_open(), bpg_decoder_close)
{
    if (!ctx)
        throw runtime_error ("bpg_decoder_open() failed");
}


void Decoder::DecodeBuffer (const void *buf, size_t len, uint8_t opts)
{
    Benchmark bm ("BPG decode");
    BPGDecoderContext *_ctx = ctx.get();

    if (opts & OPT_HEADER_ONLY)
    {
        FAIL_THROW (bpg_decoder_decode_header_only (_ctx, (uint8_t*)buf, len));
    }
    else
    {
        FAIL_THROW (bpg_decoder_decode (_ctx, (uint8_t*)buf, len));
    }

    FAIL_THROW (bpg_decoder_get_info (_ctx, &info));
}


void Decoder::DecodeFile (const char s_file[], uint8_t opts)
{
    pFILE fp (fopen (s_file, "rb"), fclose);
    FILE *_fp = fp.get();

    if (!_fp)
        throw runtime_error (std::string("Cannot open file: ") + s_file);

    fseek (_fp, 0, SEEK_END);
    size_t fsize = ftell (_fp);
    fseek (_fp, 0, SEEK_SET);

    vector<uint8_t> buf (fsize);

    if (fsize != fread (&buf[0], 1, fsize, _fp))
        throw runtime_error ("Failed to read file");

    DecodeBuffer (&buf[0], fsize, opts);
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

    {
        enum AVPixelFormat src_fmt;

        src_fmt = info.GetAVPixFmt();
        if (src_fmt < 0)
            return -1;

        swsCtx.Alloc (info.width, info.height, src_fmt, dst_fmt, quality);
    }

    {
        int src_cs;
        int src_fullRng;
        int brightness;
        int contrast;

        /* Specify color space */
        switch (info.color_space)
        {
        case BPG_CS_YCbCr:        src_cs = SWS_CS_DEFAULT; break;
        case BPG_CS_YCbCr_BT709:  src_cs = SWS_CS_ITU709;  break;
        case BPG_CS_YCbCr_BT2020: src_cs = SWS_CS_BT2020;  break;
        default:                  src_cs = SWS_CS_DEFAULT; break;
        }

        /* White-black range */
        src_fullRng = 1;
        brightness = 0;
        contrast = 1 << 16;

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
                brightness = -(16 << 8) * 219 / 255;
                contrast   = (1 << 16) * 255 / 219;
            }
            else
            {
                /* YUV -> RGB */
                src_fullRng = 0;
            }
        }

        swsCtx.setColorSpace (
            src_cs, src_fullRng,
            SWS_CS_DEFAULT, 1,
            brightness, contrast
        );
    }

    {
        const uint8_t *src[4];
        int src_stride[4];

        /* Framebuffer */
        for (int i = 0; i < 4; i++)
            src[i] = bpg_decoder_get_data (ctx.get(), src_stride+i, i);

        return swsCtx.scaleMT (*gThreadPool, src, src_stride, 0, info.height, (uint8_t**)&dst, &dst_stride);
    }
}


/**
 * Convert to a frame buffer
 */
int Decoder::ConvertToFrame (Frame &frame, int quality)
{
    enum AVPixelFormat dst_fmt;
    void *dst;
    int dst_stride;

    frame.AllocByBpp (info.width, info.height, info.GetBpp());
    dst = frame.ptr;
    dst_stride = frame.stride;

    switch (info.GetBpp())
    {
    case 8:  dst_fmt = AV_PIX_FMT_GRAY8; break;
    case 24: dst_fmt = AV_PIX_FMT_RGB24; break;
    case 32: dst_fmt = AV_PIX_FMT_RGBA;  break;
    default: dst_fmt = AV_PIX_FMT_NONE;  break;
    }

    if (dst_fmt == AV_PIX_FMT_NONE)
        return 0;

    return Convert (dst_fmt, dst, dst_stride, quality);
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

/**
 * Get corresponding AVPixelFormat
 */
enum AVPixelFormat ImageInfo::GetAVPixFmt() const
{
    switch (format)
    {
    case BPG_FORMAT_GRAY:
        return AV_PIX_FMT_GRAY16BE;

    case BPG_FORMAT_420:
    case BPG_FORMAT_420_VIDEO:
        if (has_alpha)
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUVA420P16BE;
            case 9:  return AV_PIX_FMT_YUVA420P9LE;
            case 10: return AV_PIX_FMT_YUVA420P10LE;
            default: break;
            }
        }
        else
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUV420P16BE;
            case 9:  return AV_PIX_FMT_YUV420P9LE;
            case 10: return AV_PIX_FMT_YUV420P10LE;
            case 12: return AV_PIX_FMT_YUV420P12LE;
            case 14: return AV_PIX_FMT_YUV420P14LE;
            default: break;
            }
        }
        break;

    case BPG_FORMAT_422:
    case BPG_FORMAT_422_VIDEO:
        if (has_alpha)
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUVA422P16BE;
            case 9:  return AV_PIX_FMT_YUVA422P9LE;
            case 10: return AV_PIX_FMT_YUVA422P10LE;
            default: break;
            }
        }
        else
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUV422P16BE;
            case 9:  return AV_PIX_FMT_YUV422P9LE;
            case 10: return AV_PIX_FMT_YUV422P10LE;
            case 12: return AV_PIX_FMT_YUV422P12LE;
            case 14: return AV_PIX_FMT_YUV422P14LE;
            default: break;
            }
        }
        break;
    case BPG_FORMAT_444:
        if (color_space == BPG_CS_RGB)
        {
            if (has_alpha)
                return AV_PIX_FMT_GBRAP16BE;
            else
                return AV_PIX_FMT_GBRP16BE;
        }
        else if (has_alpha)
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUVA444P16BE;
            case 9:  return AV_PIX_FMT_YUVA444P9LE;
            case 10: return AV_PIX_FMT_YUVA444P10LE;
            default: break;
            }
        }
        else
        {
            switch (bit_depth)
            {
            case 8:  return AV_PIX_FMT_YUV444P16BE;
            case 9:  return AV_PIX_FMT_YUV444P9LE;
            case 10: return AV_PIX_FMT_YUV444P10LE;
            case 12: return AV_PIX_FMT_YUV444P12LE;
            case 14: return AV_PIX_FMT_YUV444P14LE;
            default: break;
            }
        }
        break;

    default:
        break;
    }
    return AV_PIX_FMT_NONE;
}
