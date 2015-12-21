/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <windows.h>

#include "bpg_common.hpp"
#include "threadpool.hpp"


#define GETBYTE(val,n)      (((val) >> ((n) * 8)) & 0xFF)

using namespace std;
using namespace bpg;


ThreadPool *gThreadPool;


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


Decoder::Decoder(): ctx(bpg_decoder_open(), bpg_decoder_close)
{
    if (!ctx)
        throw runtime_error ("bpg_decoder_open");
}


void Decoder::DecodeBuffer (const void *buf, size_t len, uint8_t opts)
{
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


void Decoder::Start (BPGDecoderOutputFormat out_fmt, const int8_t *shuffle, bool hq_output)
{
    FAIL_THROW (bpg_decoder_start (ctx.get(), out_fmt, shuffle, hq_output));
}



DecodeBuffer::DecodeBuffer (Decoder *dec, uint16_t y_start):
    ctx(dec->GetContext()),
    buf(bpg_decoder_alloc_buffer (ctx.get(), y_start), bpg_decoder_free_buffer),
    y(y_start)
{
}


void DecodeBuffer::GetLine (void *dst)
{
    FAIL_THROW (bpg_decoder_get_line (ctx.get(), buf.get(), dst));
    y++;
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
