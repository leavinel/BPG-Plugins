/**
 * @file
 * BPG encoder / decoder C++ wrapper class
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#ifndef _XBPG_H_
#define _XBPG_H_


#include <stdint.h>
#include <stdexcept>

extern "C" {
#include "libbpg.h"
#include "bpgenc.h"
}

void pr_err (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));
void dprintf (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));

#define FAIL_THROW(expr) \
    if (expr) \
        throw runtime_error (#expr " failed")



class BpgDecoder
{
private:
    BPGDecoderContext *ctx;

public:
    BpgDecoder();
    ~BpgDecoder();

    void Decode (const uint8_t *buf, size_t len);
    void GetInfo (BPGImageInfo &info);
    void Start (BPGDecoderOutputFormat out_fmt);
    void GetLine (void *buf);
};


/**
 * BPG file reader
 */
class BpgReader
{
private:
    BpgDecoder decoder;
    uint8_t *buf;

    enum {
        FORMAT_GRAY,
        FORMAT_RGB,
        FORMAT_RGBA,
    } fmt;

public:
    BPGImageInfo info;
    uint8_t bitsPerPixel;
    size_t linesz;

    BpgReader (const char s_file[]);
    ~BpgReader();
    void GetLine (int line, void *dst) const;
};


class BpgEncParam
{
private:
    BPGEncoderParameters *pparam;
    int bitDepth;

public:
    BpgEncParam();
    ~BpgEncParam();

    BPGEncoderParameters* operator->() {
        return pparam;
    }

    operator BPGEncoderParameters*() const {
        return pparam;
    }

    int GetBitDepth() const {
        return bitDepth;
    }
};


class BpgEncImage
{
private:
    Image *img;
    uint8_t *in_buf;
    size_t buf_linesz;

public:
    BpgEncImage (int w, int h,
        BPGImageFormatEnum fmt, BPGColorSpaceEnum cs, int bit_depth);
    ~BpgEncImage();

    operator Image*() const {
        return img;
    }

    void PutLine (int line, const void *rgb);
    void Finish();
};


class BpgEncoder
{
private:
    BPGEncoderContext *ctx;
    static int WriteFunc (void *opaque, const uint8_t *buf, int buf_len);

public:
    BpgEncoder (const BpgEncParam &param);
    ~BpgEncoder();

    void Encode (FILE *fp, const BpgEncImage &img);
};


class BpgWriter
{
private:
    BpgEncParam param;
    BpgEncImage *img;
    FILE *fp;

public:
    BpgWriter (const char s_file[], int w, int h, uint8_t bits_Per_pixel);
    ~BpgWriter();

    void PutLine (int line, const void *rgb) {
        img->PutLine (line, rgb);
    }

    void Write();
};


#endif /* _XBPG_H_ */
