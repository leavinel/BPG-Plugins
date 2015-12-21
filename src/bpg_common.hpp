/**
 * @file
 * BPG encoder / decoder C++ wrapper class
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#ifndef _BPG_COMMON_HPP_
#define _BPG_COMMON_HPP_


#include <stdint.h>

#include <stdexcept>
#include <string>
#include <memory>

#include "bpg_def.h"


#define EXTC extern "C"


extern "C" {
#include "libbpg.h"
#include "bpgenc.h"
}


#include "threadpool.hpp"


#define MIN_LINES_PER_TASK      32


#define FAIL_THROW(expr) \
    if (expr) \
        throw runtime_error (#expr " failed")


extern ThreadPool *gThreadPool;


namespace bpg {

/**
 * Wrapper of #BPGImageInfo
 */
struct ImageInfo: public BPGImageInfo
{
    ImageInfo(){}

    void LoadFromBuffer (const void *buf, size_t len);
    uint8_t GetBpp() const;
    void GetFormatDetail (char buf[], size_t sz) const;
    void GetFormatDetail (std::string &s) const;

    enum {
        HEADER_MAGIC_SIZE = 4,
    };
    static bool CheckHeader (const void *buf, size_t len);
};


/* Alias of shared pointers */
typedef std::shared_ptr<BPGDecoderContext> pBPGDecoderContext;
typedef std::shared_ptr<BPGDecodeBuffer> pBPGDecoderBuffer;


/**
 * Wrapper of #BPGDecoderContext
 */
class Decoder
{
private:
    std::shared_ptr<BPGDecoderContext> ctx;
    ImageInfo info;

public:
    enum {
        OPT_HEADER_ONLY = 1,
    };

    Decoder();

    void DecodeFile (const char *s_file, uint8_t opts = 0);
    void DecodeBuffer (const void *buf, size_t len, uint8_t opts = 0);

    const pBPGDecoderContext &GetContext() const { return ctx; }
    const ImageInfo& GetInfo() const { return info; }

    void Start (BPGDecoderOutputFormat out_fmt, const int8_t *shuffle, bool hq_output = true);
};


/**
 * Wrapper of #BPGDecoderBuffer
 */
class DecodeBuffer
{
private:
    std::shared_ptr<BPGDecoderContext> ctx;
    std::shared_ptr<BPGDecodeBuffer> buf;
    uint16_t y;

public:
    DecodeBuffer (Decoder *dec, uint16_t y_start);

    void GetLine (void *buf);
};



/**
 * RGB color
 */
struct Color {
    uint8_t r, g, b;

    inline
    Color& set (uint8_t r, uint8_t g, uint8_t b) {
        this->r = r;
        this->g = g;
        this->b = b;
        return *this;
    }
};


/**
 * Color palette
 */
class Palette
{
private:
    Color colors[256];

public:
    Color& operator [] (size_t i) {
        return colors[i];
    }
};


/**
 * Input format
 */
enum EncInputFormat {
    INPUT_GRAY8 = 0,
    INPUT_RGB8,
    INPUT_RGB24,
    INPUT_RGBA32,
};


/**
 * Wrapper of #BPGEncoderParameters
 */
class EncParam
{
private:
    std::shared_ptr<BPGEncoderParameters> param;

public:
    BPGColorSpaceEnum cs;

    int BitDepth;   ///< Bit-depth of encoded BPG
                    ///< It should be added into #BPGEncoderParameters, but i didn't

    EncParam();

    void ParseParam (const char s_buf[]);
    void LoadConfig (const char s_file[], uint8_t bits_per_pixel);

    BPGEncoderParameters* operator->() {
        return param.get();
    }

    operator BPGEncoderParameters*() const {
        return param.get();
    }
};


/**
 * Wrapper of #Image structure
 * The image buffer for BPG encoding
 */
class EncImage
{
private:
    std::shared_ptr<Image> img;

public:
    EncImage (int w, int h, BPGImageFormatEnum fmt, bool has_alpha, BPGColorSpaceEnum cs, int bit_depth);

    operator Image*() const {
        return img.get();
    }

    void Convert (const void *dat);
};


/**
 * Image buffer for encoding
 */
class EncImageBuffer
{
private:
    int w, h;
    EncInputFormat inFmt;
    BPGImageFormatEnum outFmt;
    bool hasAlpha;
    std::unique_ptr<uint8_t[]> buf;
    size_t bufStride;      ///< Stride of buffer

public:
    EncImageBuffer (int w, int h, EncInputFormat in_fmt);

    void PutLine (int line, const void *rgb, Palette *pal = NULL);
    EncImage *CreateImage (BPGColorSpaceEnum out_cs, int bit_depth);
};




/**
 * Wrapper of #BPGEncoderContext
 */
class Encoder
{
private:
    std::shared_ptr<BPGEncoderContext> ctx;
    static int WriteFunc (void *opaque, const uint8_t *buf, int buf_len);

public:
    Encoder (const EncParam &param);
    void Encode (FILE *fp, const EncImage &img);
};


} // namespace bpg

#endif /* _BPG_COMMON_HPP_ */
