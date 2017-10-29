/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <windows.h>
#include <psapi.h>

extern "C" {
#include "libswscale/swscale.h"
}

#include "bpg_common.hpp"
#include "log.h"

using namespace std;
using namespace bpg;


static int get_param (const char buf[], const char s_opt[], const char s_fmt[], ...)
__attribute__((format (scanf, 3, 4)));

static int get_param (const char buf[], const char s_opt[], const char s_fmt[], ...)
{
    const char *target = strstr (buf, s_opt);
    if (!target)
        return 0;

    va_list ap;
    va_start (ap, s_fmt);
    int ret = vsscanf (target, s_fmt, ap);
    va_end (ap);
    return ret;
}


IniFile::IniFile(): fp(nullptr, fclose)
{
}


/** Get path of this DLL */
void IniFile::getPath (string &s)
{
    char s_dll_dir[256];

    GetModuleFileNameA (NULL, s_dll_dir, sizeof(s_dll_dir));
    char *s_slash = strrchr (s_dll_dir, '\\');
    s_slash[1] = '\0';

    s = s_dll_dir;
}


/** Open an INI file */
void IniFile::Open (const char *s_file)
{
    string s_ini;

    getPath (s_ini);
    s_ini += s_file;

    Logi ("Open INI file '%s'...\n", s_ini.c_str());

    fp = pFILE (
        fopen (s_ini.c_str(), "rb"),
        fclose
    );
}


/** Get the line by specified prefix */
void IniFile::GetLineByPrefix (string &s, const char s_prefix[])
{
    s.clear();
    if (!fp)
        return;

    FILE *f = fp.get();
    char buf[256];

    rewind (f);

    while (fgets (buf, sizeof(buf), f))
    {
        if (0 == memcmp (buf, s_prefix, strlen(s_prefix))) // Found
        {
            s = buf;
            break;
        }
    }
}


EncParam::EncParam():
    param (bpg_encoder_param_alloc(), bpg_encoder_param_free)
{
}

/**
 * Parse encoding parameters with option string
 */
void EncParam::Parse (const char s_opt[])
{
    /* QP */
    if (get_param (s_opt, "-q", "-q %d", &param->qp))
        Logi ("-q %d\n", param->qp);

    /* Chroma format */
    {
        int tmp = 0;
        if (get_param (s_opt, "-f", "-f %d", &tmp))
        {
            switch (tmp)
            {
            case 420:
                param->preferred_chroma_format = BPG_FORMAT_420;
                break;
            case 422:
                param->preferred_chroma_format = BPG_FORMAT_422;
                break;
            case 444:
                param->preferred_chroma_format = BPG_FORMAT_444;
                break;
            default:
                throw runtime_error ("invalid chroma format");
            }

            Logi ("-f %d\n", tmp);
        }
    }

    /* Encoder */
    param->encoder_type = HEVC_ENCODER_X265; // Always use x265

    /* Compression level */
    if (get_param (s_opt, "-m", "-m %d", &param->compress_level))
        Logi ("-m %d\n", param->compress_level);

    /* Loseless mode */
    if (strstr (s_opt, "-loseless"))
    {
        param->lossless = 1;
        Logi ("%s", "-loseless\n");
    }
}


typedef std::unique_ptr<Image, void(*)(Image*)> pImage;

/**
 * Wrapper of #Image structure
 * The image buffer (YUV) for BPG encoding
 */
class encImage
{
private:
    pImage img;
    enum AVPixelFormat dst_fmt;

public:
    encImage(): img(nullptr, image_free), dst_fmt(AV_PIX_FMT_NONE) {}

    operator bool() const { return !!img; }
    Image* get() { return img.get(); }

    void Alloc (const EncParam &param, const FrameDesc &frame);
    void Convert (const EncParam &param, const FrameDesc &frame);
};



void encImage::Alloc (const EncParam &param, const FrameDesc &frame)
{
    BPGImageFormatEnum fmt2;
    int has_alpha;

    switch (frame.fmt)
    {
        case AV_PIX_FMT_GRAY8:
            fmt2 = BPG_FORMAT_GRAY;
            dst_fmt = AV_PIX_FMT_GRAY16BE;
            has_alpha = 0;
            break;
        case AV_PIX_FMT_RGB24:
        case AV_PIX_FMT_BGR24:
            fmt2 = param->preferred_chroma_format;
            switch (fmt2)
            {
                case BPG_FORMAT_420: dst_fmt = AV_PIX_FMT_YUV420P16BE; break;
                case BPG_FORMAT_422: dst_fmt = AV_PIX_FMT_YUV422P16BE; break;
                case BPG_FORMAT_444: dst_fmt = AV_PIX_FMT_YUV444P16BE; break;
                default: throw runtime_error ("invalid chroma format");
            }
            has_alpha = 0;
            break;
        case AV_PIX_FMT_RGBA:
        case AV_PIX_FMT_BGRA:
            fmt2 = param->preferred_chroma_format;
            switch (fmt2)
            {
                case BPG_FORMAT_420: dst_fmt = AV_PIX_FMT_YUVA420P16BE; break;
                case BPG_FORMAT_422: dst_fmt = AV_PIX_FMT_YUVA422P16BE; break;
                case BPG_FORMAT_444: dst_fmt = AV_PIX_FMT_YUVA444P16BE; break;
                default: throw runtime_error ("invalid chroma format");
            }
            has_alpha = 1;
            break;
        default:
            throw runtime_error ("invalid frame format");
    }

    switch (frame.fmt)
    {
        case AV_PIX_FMT_RGBA:
        case AV_PIX_FMT_BGRA:
            has_alpha = 1;
            break;
        default:
            has_alpha = 0;
            break;
    }

    img = pImage (
        image_alloc (frame.w, frame.h, fmt2, has_alpha, param.cs, param.BitDepth),
        image_free
    );

    img->limited_range = 0;
}


/**
 * Convert BPG encoding image (YUV) from Frame
 */
void encImage::Convert (const EncParam &param, const FrameDesc &frame)
{
    sws::Context swsCtx;
    const uint8_t *src;
    int src_stride;
    uint8_t *dst[4];
    int dst_stride[4];

    Logi ("Converting color format...\n");
    src = (const uint8_t*)frame.ptr;
    src_stride = frame.stride;

    for (int i = 0; i < 4; i++)
    {
        dst[i] = img->data[i];
        dst_stride[i] = img->linesize[i];
    }

    swsCtx.Alloc (frame.w, frame.h, frame.fmt, dst_fmt);
    swsCtx.setColorSpace (SWS_CS_DEFAULT, 1, SWS_CS_DEFAULT, 1);
    swsCtx.scaleMT (*gThreadPool, &src, &src_stride, 0, frame.h, dst, dst_stride);
}


int Encoder::writeFunc (void *opaque, const uint8_t *buf, int buf_len)
{
    FILE *fp = (FILE*)opaque;

    size_t len = fwrite (buf, 1, buf_len, fp);
    Logi ("Written %d/%d bytes...\n", buf_len, len);
    return len;
}


void Encoder::Encode (FILE *fp, const EncParam &param, const FrameDesc &frame)
{
    pBPGEncoderContext ctx (
        bpg_encoder_open (param.get()),
        bpg_encoder_close
    );

    if (!ctx)
        throw runtime_error ("Encoder parameter not set");

    encImage img;
    img.Alloc (param, frame);
    img.Convert (param, frame);

    Logi ("Encoding...\n");
    FAIL_THROW (bpg_encoder_encode (ctx.get(), img.get(), writeFunc, fp));
    Logi ("Done\n");
}
