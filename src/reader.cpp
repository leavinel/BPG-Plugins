/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include <windows.h>

#include "bpg_common.h"

#define GETBYTE(val,n)      (((val) >> ((n) * 8)) & 0xFF)

using namespace std;


/**
 * Create a BPG image info from buffer
 */
BpgImageInfo2::BpgImageInfo2 (const void *buf, size_t len)
{
    FAIL_THROW (bpg_decoder_get_info_from_buf (this, NULL, (uint8_t*)buf, len));
}


uint8_t BpgImageInfo2::GetBpp() const
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
bool BpgImageInfo2::CheckHeader (const void *buf, size_t len)
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


BpgDecoder::BpgDecoder(): ctx(NULL)
{
    ctx = bpg_decoder_open();
    if (!ctx)
        throw runtime_error ("bpg_decoder_open");
}

BpgDecoder::~BpgDecoder()
{
    if (ctx)
    {
        bpg_decoder_close (ctx);
        ctx = NULL;
    }
}

void BpgDecoder::Decode (BpgImageInfo2 &info, const void *buf, size_t len)
{
    FAIL_THROW (bpg_decoder_decode (ctx, (uint8_t*)buf, len));
    FAIL_THROW (bpg_decoder_get_info (ctx, &info));
}

void BpgDecoder::Start (BPGDecoderOutputFormat out_fmt)
{
    FAIL_THROW (bpg_decoder_start (ctx, out_fmt));
}

void BpgDecoder::GetLine (void *buf)
{
    FAIL_THROW (bpg_decoder_get_line (ctx, buf));
}


uint8_t *BpgReader::decodeBuf = NULL;
size_t BpgReader::bufsz = 0;


void BpgReader::InitClass()
{
}


void BpgReader::DeinitClass()
{
    if (decodeBuf)
    {
        delete[] decodeBuf;
        decodeBuf = NULL;
    }
}


BpgReader::BpgReader(): fmt(FORMAT_RGB), bitsPerPixel(0), linesz(0)
{
}


BpgReader::~BpgReader()
{
}


void BpgReader::LoadFromBuffer (const void *buf, size_t len)
{
    DWORD begin = GetTickCount();
    decoder.Decode (info, buf, len);

    BPGDecoderOutputFormat out_fmt;

    bitsPerPixel = info.GetBpp();
    switch (bitsPerPixel)
    {
    case 8:
        fmt = FORMAT_GRAY;
        out_fmt = BPG_OUTPUT_FORMAT_RGB24;
        break;

    case 24:
        fmt = FORMAT_RGB;
        out_fmt = BPG_OUTPUT_FORMAT_RGB24;
        break;

    case 32:
        fmt = FORMAT_RGBA;
        out_fmt = BPG_OUTPUT_FORMAT_RGBA32;
        break;

    default:
        break;
    }

    if (BPG_OUTPUT_FORMAT_RGB24)
        linesz = info.width * 3;
    else
        linesz = info.width * 4;

    /* Reallocate buffer if required */
    size_t min_bufsz = linesz * info.height;
    if (bufsz < min_bufsz)
    {
        if (decodeBuf)
            delete decodeBuf;

        decodeBuf = new uint8_t [min_bufsz];
        bufsz = min_bufsz;
    }

    uint8_t *p = decodeBuf;

    decoder.Start (out_fmt);
    for (uint32_t y = 0; y < info.height; y++)
    {
        decoder.GetLine (p);
        p += linesz;
    }

    DWORD dur = GetTickCount() - begin;
    dprintf ("Decoding time: %lu ms\n", dur);
}


void BpgReader::LoadFromFile (const char s_file[])
{
    FILE *fp = fopen (s_file, "rb");

    if (!fp)
        throw runtime_error (std::string("Cannot open file: ") + s_file);

    fseek (fp, 0, SEEK_END);
    size_t fsize = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    uint8_t *buf = new uint8_t [fsize];

    if (fsize != fread (buf, 1, fsize, fp))
        throw runtime_error ("Failed to read file");

    fclose (fp);

    try {
        LoadFromBuffer (buf, fsize);
        delete buf;
    }
    catch (const exception &e) {
        delete buf;
        throw e;
    }
}


void BpgReader::GetFormatDetail (char buf[], size_t sz) const
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

    snprintf (buf, sz, "BPG %s %s", s_fmt[info.format], s_cs[info.color_space]);
}


void BpgReader::GetFormatDetail (string &s) const
{
    char buf[128];
    GetFormatDetail (buf, sizeof(buf));
    s = buf;
}


void BpgReader::GetLine (int line, void *_dst, const uint8_t rgba_offset[]) const
{
    uint8_t *dst = (uint8_t*)_dst;
    const uint8_t *src = decodeBuf;
    src += linesz * line;

    switch (fmt)
    {
    case FORMAT_GRAY: // RGB24 => grayscale
        for (size_t i = 0; i < info.width; i++)
        {
            *(dst++) = *src;
            src += 3;
        }
        break;

    case FORMAT_RGB:
        if (rgba_offset == NULL) // By default (RGB)
        {
            memcpy (dst, src, linesz);
        }
        else
        {
            int offr = rgba_offset[0];
            int offg = rgba_offset[1];
            int offb = rgba_offset[2];

            for (size_t i = 0; i < info.width; i++)
            {
                dst[offr] = *(src++);
                dst[offg] = *(src++);
                dst[offb] = *(src++);
                dst += 3;
            }
        }
        break;

    case FORMAT_RGBA:
        if (rgba_offset == NULL) // By default (RGBA)
        {
            memcpy (dst, src, linesz);
        }
        else
        {
            uint8_t offr = rgba_offset[0];
            uint8_t offg = rgba_offset[1];
            uint8_t offb = rgba_offset[2];
            uint8_t offa = rgba_offset[3];

            for (size_t i = 0; i < info.width; i++)
            {
                dst[offr] = *(src++);
                dst[offg] = *(src++);
                dst[offb] = *(src++);
                dst[offa] = *(src++);
                dst += 4;
            }
        }
        break;

    default:
        break;
    }
}
