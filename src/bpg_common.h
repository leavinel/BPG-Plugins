/**
 * @file
 * BPG encoder / decoder C++ wrapper class
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#ifndef _BPG_COMMON_H_
#define _BPG_COMMON_H_


#include <stdint.h>
#include <stdexcept>
#include <string>
#include "bpg_def.h"


#define EXTC extern "C"


extern "C" {
#include "libbpg.h"
#include "bpgenc.h"
}

void pr_err (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));
void dprintf (const char s_fmt[], ...) __attribute__((format(printf, 1, 2)));

#define FAIL_THROW(expr) \
    if (expr) \
        throw runtime_error (#expr " failed")


struct BpgImageInfo2: public BPGImageInfo
{
    BpgImageInfo2(){}

    void LoadFromBuffer (const void *buf, size_t len);
    uint8_t GetBpp() const;
    void GetFormatDetail (char buf[], size_t sz) const;
    void GetFormatDetail (std::string &s) const;

    enum {
        HEADER_MAGIC_SIZE = 4,
    };
    static bool CheckHeader (const void *buf, size_t len);
};


class BpgDecoder
{
private:
    BPGDecoderContext *ctx;
    BpgImageInfo2 info;

public:
    BpgDecoder();
    ~BpgDecoder();

    void Decode (const void *buf, size_t len);
    const BpgImageInfo2& GetInfo() const { return info; }
    void Start (BPGDecoderOutputFormat out_fmt, const int8_t *shuffle);
    void GetLine (int y, void *buf);
};


/**
 * BPG file reader
 */
class BpgReader
{
private:
    BpgDecoder decoder;

public:
    static void InitClass();
    static void DeinitClass();

    BpgReader(){}
    ~BpgReader(){}
    void LoadFromFile (const char s_file[]);
    void LoadFromBuffer (const void *buf, size_t len);
    BpgDecoder& GetDecoder() { return decoder; }
};


class BpgEncParam
{
public:
private:
    BPGEncoderParameters *pparam;

public:
    int BitDepth;

    BpgEncParam();
    ~BpgEncParam();

    void ParseParam (const char s_buf[]);

    BPGEncoderParameters* operator->() {
        return pparam;
    }

    operator BPGEncoderParameters*() const {
        return pparam;
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


#endif /* _BPG_COMMON_H_ */
