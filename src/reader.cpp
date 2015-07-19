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
 * Load a BPG image info from buffer
 */
void BpgImageInfo2::LoadFromBuffer (const void *buf, size_t len)
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

void BpgDecoder::Decode (const void *buf, size_t len)
{
    DWORD begin = GetTickCount();
    FAIL_THROW (bpg_decoder_decode (ctx, (uint8_t*)buf, len));
    dprintf ("bpg_decoder_decode time: %lu ms\n", GetTickCount() - begin);
    FAIL_THROW (bpg_decoder_get_info (ctx, &info));
}

void BpgDecoder::Start (BPGDecoderOutputFormat out_fmt, const int8_t *shuffle)
{
    FAIL_THROW (bpg_decoder_start (ctx, out_fmt, shuffle));
}

void BpgDecoder::GetLine (int y, void *buf)
{
    FAIL_THROW (bpg_decoder_get_line (ctx, y, buf));
}


void BpgReader::InitClass()
{
}


void BpgReader::DeinitClass()
{
}


void BpgReader::LoadFromBuffer (const void *buf, size_t len)
{
    decoder.Decode (buf, len);
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


void BpgImageInfo2::GetFormatDetail (char buf[], size_t sz) const
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


void BpgImageInfo2::GetFormatDetail (string &s) const
{
    char buf[128];
    GetFormatDetail (buf, sizeof(buf));
    s = buf;
}
