/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <string.h>
#include <string>

#include "Xbpg.h"

using namespace std;


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

void BpgDecoder::Decode (const uint8_t *buf, size_t len)
{
    FAIL_THROW (bpg_decoder_decode (ctx, buf, len));
}

void BpgDecoder::GetInfo (BPGImageInfo &info)
{
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


BpgReader::BpgReader (const char s_file[]): buf(NULL), linesz(0)
{
    do {
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

        decoder.Decode (buf, fsize);
        decoder.GetInfo (info);
    }
    while (0);

    BPGDecoderOutputFormat out_fmt;

    if (info.format == BPG_FORMAT_GRAY)
    {
        fmt = FORMAT_GRAY;
        out_fmt = BPG_OUTPUT_FORMAT_RGB24;
        bitsPerPixel = 8;
    }
    else if (info.has_alpha)
    {
        fmt = FORMAT_RGBA;
        out_fmt = BPG_OUTPUT_FORMAT_RGBA32;
        bitsPerPixel = 32;
    }
    else
    {
        fmt = FORMAT_RGB;
        out_fmt = BPG_OUTPUT_FORMAT_RGB24;
        bitsPerPixel = 24;
    }

    if (BPG_OUTPUT_FORMAT_RGB24)
        linesz = info.width * 3;
    else
        linesz = info.width * 4;

    buf = new uint8_t [linesz * info.height];
    uint8_t *p = buf;

    decoder.Start (out_fmt);
    for (uint32_t y = 0; y < info.height; y++)
    {
        decoder.GetLine (p);
        p += linesz;
    }
}


BpgReader::~BpgReader()
{
    if (buf)
    {
        delete[] buf;
        buf = NULL;
    }
}


void BpgReader::GetLine (int line, void *_dst) const
{
    uint8_t *dst = (uint8_t*)_dst;
    const uint8_t *src = buf;
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

    default:
        memcpy (dst, src, linesz);
        break;
    }
}
