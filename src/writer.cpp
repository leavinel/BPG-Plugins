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

#include "Xbpg.h"

#define INI_PATH        "Plugins\\Xbpg.ini"

using namespace std;


static void get_ini_path (string &path)
{
    char s_dll_dir[256];
    GetModuleFileNameA (NULL, s_dll_dir, sizeof(s_dll_dir));
    char *s_slash = strrchr (s_dll_dir, '\\');
    s_slash[1] = '\0';

    path = s_dll_dir;
    path += INI_PATH;
}


static FILE* open_ini_file()
{
    string s_ini;
    get_ini_path (s_ini);
    dprintf ("Open INI file '%s'...\n", s_ini.c_str());
    return fopen (s_ini.c_str(), "rb");
}


static void get_param (const char buf[], const char s_opt[], const char s_fmt[], ...)
__attribute__((format (scanf, 3, 4)));

static void get_param (const char buf[], const char s_opt[], const char s_fmt[], ...)
{
    const char *target = strstr (buf, s_opt);
    if (!target)
        return;

    va_list ap;
    va_start (ap, s_fmt);
    vsscanf (target, s_fmt, ap);
    va_end (ap);
}


void BpgEncParam::loadParam()
{
    FILE *fp;
    char line[64];

    fp = open_ini_file();
    if (!fp)
    {
        dprintf ("%s", "Cannot open INI file\n");
        return;
    }

    fgets (line, sizeof(line), fp);
    fclose (fp);
    dprintf ("option: %s\n", line);

    /* QP */
    get_param (line, "-q", "-q %d", &pparam->qp);
    dprintf ("-q %d\n", pparam->qp);

    /* Chroma format */
    {
        int tmp = 0;
        get_param (line, "-f", "-f %d", &tmp);
        dprintf ("-f %d\n", tmp);

        switch (tmp)
        {
        case 420:
            pparam->preferred_chroma_format = BPG_FORMAT_420;
            break;
        case 422:
            pparam->preferred_chroma_format = BPG_FORMAT_422;
            break;
        case 444:
            pparam->preferred_chroma_format = BPG_FORMAT_444;
            break;
        default:
            throw runtime_error ("invalid chroma format");
        }
    }

    /* Bit depth */
    get_param (line, "-b", "-b %d", &bitDepth);
    dprintf ("-b %d\n", bitDepth);

    /* Encoder */
    {
        char buf[8];
        get_param (line, "-e", "-e %7s", buf);

        if (strcmp ("jctvc", buf) == 0)
            pparam->encoder_type = HEVC_ENCODER_JCTVC;
        else if (strcmp ("x265", buf) == 0)
            pparam->encoder_type = HEVC_ENCODER_X265;

        const char *s_enc = NULL;
        switch (pparam->encoder_type)
        {
        case HEVC_ENCODER_JCTVC:
            s_enc = "JCTVC";
            break;
        case HEVC_ENCODER_X265:
            s_enc = "X265";
            break;
        default:
            throw runtime_error ("invalid encoder type");
        }

        dprintf ("Encoder: %s\n", s_enc);
    }

    /* Compression level */
    get_param (line, "-m", "-m %d", &pparam->compress_level);
    dprintf ("-m %d\n", pparam->compress_level);
}


BpgEncParam::BpgEncParam(): pparam(NULL), bitDepth(8)
{
    pparam = bpg_encoder_param_alloc();
    loadParam();
}

BpgEncParam::~BpgEncParam()
{
    bpg_encoder_param_free (pparam);
}


BpgEncoder::BpgEncoder (const BpgEncParam &param): ctx(NULL)
{
    ctx = bpg_encoder_open (param);
}

BpgEncoder::~BpgEncoder()
{
    bpg_encoder_close (ctx);
}


int BpgEncoder::WriteFunc (void *opaque, const uint8_t *buf, int buf_len)
{
    FILE *fp = (FILE*)opaque;

    size_t len = fwrite (buf, 1, buf_len, fp);
    dprintf ("Written %d/%d bytes...\n", buf_len, len);
    return len;
}

#include <windows.h>
void BpgEncoder::Encode (FILE *fp, const BpgEncImage &img)
{
    dprintf ("Encoding...\n");
    FAIL_THROW (bpg_encoder_encode (ctx, img, WriteFunc, fp));
    dprintf ("Done\n");
}


BpgEncImage::BpgEncImage (int w, int h,
    BPGImageFormatEnum fmt, BPGColorSpaceEnum cs, int bit_depth):
    img(NULL), in_buf(NULL), buf_linesz(0)
{
    switch (fmt)
    {
    case BPG_FORMAT_GRAY:
        buf_linesz = w;
        break;
    case BPG_FORMAT_444:
        buf_linesz = w * 3;
        break;
    default:
        throw runtime_error ("invalid format");
    }

    in_buf = new uint8_t [buf_linesz * h];
    img = image_alloc (w, h, fmt, 0, cs, bit_depth);
}

BpgEncImage::~BpgEncImage()
{
    image_free (img);
    delete in_buf;
}

void BpgEncImage::PutLine (int line, const void *dat)
{
    memcpy (in_buf + line * buf_linesz, dat, buf_linesz);
}

void BpgEncImage::Finish()
{
    dprintf ("Converting color format...\n");

    switch (img->format)
    {
    case BPG_FORMAT_GRAY:
        bpg_gray8_to_img (img, in_buf);
        break;
    case BPG_FORMAT_444:
        bpg_rgb24_to_img (img, in_buf);
        break;
    default:
        break;
    }
}


BpgWriter::BpgWriter (const char s_file[], int w, int h, uint8_t bits_per_pixel)
{
    BPGImageFormatEnum fmt;

    /* x265 does not support monochrome */
    switch (bits_per_pixel)
    {
    case 8:
        fmt = BPG_FORMAT_GRAY;
        param->preferred_chroma_format = fmt;
        break;
    case 24:
        fmt = BPG_FORMAT_444;
        break;
    default:
        throw runtime_error ("Invalid bits per pixel");
    }

    img = new BpgEncImage (w, h, fmt, BPG_CS_YCbCr, param.GetBitDepth());

    dprintf ("Open %s\n", s_file);
    fp = fopen (s_file, "wb");
    if (!fp)
        throw runtime_error ("Cannot open file");
}

BpgWriter::~BpgWriter()
{
    if (img)
    {
        delete img;
        img = NULL;
    }

    if (fp)
    {
        dprintf ("Close file\n");
        fclose (fp);
        fp = NULL;
    }
}

void BpgWriter::Write()
{
    img->Finish();

    BpgEncoder enc (param);
    enc.Encode (fp, *img);
}
