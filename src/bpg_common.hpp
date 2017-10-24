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
#include "sws_context.hpp"

#undef EXT
#ifdef BPG_COMMON_SET
#define EXT
#else
#define EXT extern
#endif


#define MIN_LINES_PER_TASK      32


#define FAIL_THROW(expr) \
    if (expr) \
        throw runtime_error (#expr " failed")


EXT ThreadPool *gThreadPool;

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


/**
 * Decoded frame w/ buffer
 */
class Frame
{
private:
    bool bReady;
    std::unique_ptr<uint8_t[]> buf;
    uint32_t stride;

public:
    Frame(): bReady(false), stride(0) {}

    bool isReady() const { return bReady; }
    int GetStride() const { return stride; }
    uint8_t *GetBuffer() { return buf.get(); }

    void Alloc (const ImageInfo &info);
    void GetLine (int y, void *dst);
};



/**
 * Wrapper of #BPGDecoderContext
 */
class Decoder
{
private:
    std::shared_ptr<BPGDecoderContext> ctx;
    ImageInfo info;

    struct convertParam;
    class convertTask;

    int prepareConvert (
        convertParam &param,
        enum AVPixelFormat dst_fmt,
        void *dst,
        int dst_stride,
        int quality
    );

    int doConvert (
        sws::Context &swsCtx,
        const convertParam &param,
        int cvt_y,
        int cvt_h
    );

public:
    enum {
        OPT_HEADER_ONLY = 1,
    };

    Decoder();

    void DecodeFile (const char *s_file, uint8_t opts = 0);
    void DecodeBuffer (const void *buf, size_t len, uint8_t opts = 0);

    const ImageInfo& GetInfo() const { return info; }

    int Convert (
        enum AVPixelFormat dst_fmt,
        void *dst,
        int dst_stride,
        int quality = 9
    );

    int ConvertMT (
        ThreadPool &pool,
        enum AVPixelFormat dst_fmt,
        void *dst,
        int dst_stride,
        int quality = 9
    );

    int ConvertToFrame (Frame &frame, ThreadPool &pool, int quality = 9);
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
