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

#include "bpg_common.hpp"
#include "log.h"

using namespace std;
using namespace bpg;


/**
 * Ini file smart pointer
 */
class IniFile
{
private:
    FILE *fp;

    string getPath() {
        char s_dll_dir[256];
        GetModuleFileNameA (NULL, s_dll_dir, sizeof(s_dll_dir));
        char *s_slash = strrchr (s_dll_dir, '\\');
        s_slash[1] = '\0';

        return s_dll_dir;
    }

public:
    IniFile (const char *s_file): fp(NULL) {
        string s_ini = getPath() + s_file;
        Logi ("Open INI file '%s'...\n", s_ini.c_str());
        fp = fopen (s_ini.c_str(), "rb");
    }

    ~IniFile() {
        if (fp)
        {
            fclose (fp);
            fp = NULL;
        }
    }

    /** Test if file is opened */
    operator bool() const {
        return fp != NULL;
    }

    /** Get the line with specified prefix */
    void getLine (string &s, const char s_prefix[]) {
        s.clear();

        if (!fp)
            return;

        char buf[256];
        rewind (fp);

        while (fgets (buf, sizeof(buf), fp))
        {
            if (0 == memcmp (buf, s_prefix, strlen(s_prefix))) // Found
            {
                s = buf;
                break;
            }
        }
    }
};


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


EncParam::EncParam():
    param(bpg_encoder_param_alloc(), bpg_encoder_param_free),
    cs(BPG_CS_YCbCr), BitDepth(8)
{
}

/**
 * Parse encoding parameters with option string
 */
void EncParam::ParseParam (const char s_opt[])
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

    /* Bit depth */
    if (get_param (s_opt, "-b", "-b %d", &BitDepth))
        Logi ("-b %d\n", BitDepth);

    /* Encoder */
    param->encoder_type = HEVC_ENCODER_X265; // Always use x265
#if 0
    {
        char buf[8];
        if (get_param (s_opt, "-e", "-e %7s", buf))
        {
            if (strcmp ("jctvc", buf) == 0)
                param->encoder_type = HEVC_ENCODER_JCTVC;
            else if (strcmp ("x265", buf) == 0)
                param->encoder_type = HEVC_ENCODER_X265;
            else
                throw runtime_error ("invalid encoder type");

            Logi ("-e %s\n", buf);
        }
    }
#endif

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


/**
 * Load parameters from a configuration file
 * @param s_file
 */
void EncParam::LoadConfig (const char s_file[], uint8_t bits_per_pixel)
{
    IniFile f_ini (s_file);

    if (!f_ini)
    {
        Logi ("%s", "Cannot open INI file\n");
    }
    else
    {
        char s_prefix[8];
        snprintf (s_prefix, sizeof(s_prefix), "%u:", bits_per_pixel);

        string s_param;
        f_ini.getLine (s_param, s_prefix);
        ParseParam (s_param.c_str());
    }
}



Encoder::Encoder (const EncParam &param):
    ctx (bpg_encoder_open (param), bpg_encoder_close)
{
}


int Encoder::WriteFunc (void *opaque, const uint8_t *buf, int buf_len)
{
    FILE *fp = (FILE*)opaque;

    size_t len = fwrite (buf, 1, buf_len, fp);
    Logi ("Written %d/%d bytes...\n", buf_len, len);
    return len;
}


void Encoder::Encode (FILE *fp, const EncImage &img)
{
    Logi ("Encoding...\n");
    FAIL_THROW (bpg_encoder_encode (ctx.get(), img, WriteFunc, fp));
    Logi ("Done\n");
}


EncImage::EncImage (
    int w,
    int h,
    BPGImageFormatEnum fmt,
    bool has_alpha,
    BPGColorSpaceEnum cs,
    int bit_depth)
{
    img.reset (
        image_alloc (w, h, fmt, has_alpha ? 1 : 0, cs, bit_depth),
        image_free
    );
}


void EncImage::Convert (const void *dat)
{
    Logi ("Converting color format...\n");

    switch (img->format)
    {
    case BPG_FORMAT_GRAY:
        bpg_gray8_to_img (img.get(), dat);
        break;
    case BPG_FORMAT_444:
        bpg_rgb24_to_img (img.get(), dat);
        break;
    default:
        break;
    }
}


EncImageBuffer::EncImageBuffer (int w, int h, EncInputFormat in_fmt):
    w(w), h(h), inFmt(in_fmt), bufStride(0)
{
    switch (in_fmt)
    {
    case INPUT_GRAY8:
        bufStride = w;
        outFmt = BPG_FORMAT_GRAY;
        hasAlpha = false;
        break;

    case INPUT_RGB8:
    case INPUT_RGB24:
        bufStride = w * 3;
        outFmt = BPG_FORMAT_444;
        hasAlpha = false;
        break;

    case INPUT_RGBA32:
        bufStride = w * 4;
        outFmt = BPG_FORMAT_444;
        hasAlpha = true;
        break;
    default:
        throw runtime_error ("invalid format");
    }

    buf.reset ( new uint8_t [bufStride * h] );
}


void EncImageBuffer::PutLine (int line, const void *dat, Palette *pal)
{
    switch (inFmt)
    {
    case INPUT_RGB8:
    {
        const uint8_t *src = (uint8_t*)dat;
        Color *dst = (Color*)(buf.get() + line * bufStride);

        for (int x = 0; x < w; x++)
            *dst++ = (*pal)[src[x]];
    }
        break;

    default:
        memcpy (buf.get() + line * bufStride, dat, bufStride);
        break;
    }
}


EncImage *EncImageBuffer::CreateImage (BPGColorSpaceEnum out_cs, int bit_depth)
{
    EncImage *img = new EncImage (w, h, outFmt, hasAlpha, out_cs, bit_depth);
    img->Convert (buf.get());

    return img;
}
