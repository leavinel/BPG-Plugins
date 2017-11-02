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
#include "frame.hpp"

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


namespace bpg {

EXT ThreadPool *gThreadPool;

typedef std::unique_ptr<FILE, int(*)(FILE*)> pFILE;


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
    enum AVPixelFormat GetAVPixFmt() const;

    enum {
        HEADER_MAGIC_SIZE = 4,
    };
    static bool CheckHeader (const void *buf, size_t len);
};



typedef std::unique_ptr<BPGDecoderContext, void(*)(BPGDecoderContext*)> pBPGDecoderContext;


/**
 * Wrapper of #BPGDecoderContext
 */
class Decoder
{
private:
    pBPGDecoderContext ctx;
    ImageInfo info;

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
        int quality = -1
    );

    int ConvertToFrame (Frame &frame, int quality = -1);
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


typedef std::unique_ptr<BPGEncoderParameters, void(*)(BPGEncoderParameters*)> pBPGEncoderParameters;

/**
 * Wrapper of #BPGEncoderParameters
 */
class EncParam
{
private:
    pBPGEncoderParameters param;

public:
    static const BPGColorSpaceEnum cs = BPG_CS_YCbCr;
    static const uint8_t BitDepth = 8;

    EncParam();
    BPGEncoderParameters *operator->() const { return param.get(); }

    void Parse (const char s_opt[]);

    BPGEncoderParameters* get() { return param.get(); }
    const BPGEncoderParameters* get() const { return param.get(); }
};


/**
 * INI file reader
 */
class IniFile
{
private:
    pFILE fp;
    static void getPath (std::string &s);

public:
    IniFile();

    void Open (const char *s_file);
    operator bool() const { return !!fp; }
    void GetLineByPrefix (std::string &s, const char s_prefix[]);
};


typedef std::unique_ptr<BPGEncoderContext, void(*)(BPGEncoderContext*)> pBPGEncoderContext;

/**
 * Wrapper of #BPGEncoderContext
 */
class Encoder
{
private:
    static int writeFunc (void *opaque, const uint8_t *buf, int buf_len);

public:
    Encoder(){}
    void Encode (FILE *fp, const EncParam &param, const FrameDesc &frame);
};


} // namespace bpg

#endif /* _BPG_COMMON_HPP_ */
