/*
 * Plugin example for XnView 1.65 or more
 *
 * When Xuser is built, put it in PluginsEx folder
 */

#include <stdio.h>
#include <windows.h>

#include "bpg_common.hpp"
#include "looptask.hpp"
#include "benchmark.hpp"


#define ENC_CONFIG_FILE     "Plugins\\Xbpg.ini"


using namespace std;


EXTC BOOL APIENTRY DllMain (HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH :
        Logi ("Compiled at %s %s\n", __TIME__, __DATE__);
        gThreadPool = new ThreadPool;
        break;

    case DLL_PROCESS_DETACH :
        delete gThreadPool;
        break;

    case DLL_THREAD_ATTACH  :
    case DLL_THREAD_DETACH  :
        break;
    }

    return TRUE;
}

#define API __stdcall

#define GFP_RGB    0
#define GFP_BGR    1

#define GFP_READ    0x0001
#define GFP_WRITE 0x0002

typedef struct {
        unsigned char red[256];
        unsigned char green[256];
        unsigned char blue[256];
    } GFP_COLORMAP;


EXTC BOOL API gfpGetPluginInfo (
    DWORD version,
    LPSTR label,
    INT label_max_size,
    LPSTR extension,
    INT extension_max_size,
    INT *support
) {
    if ( version != 0x0002 )
        return FALSE;

    strncpy (label, FORMAT_NAME, label_max_size);
    strncpy (extension, FORMAT_EXT, extension_max_size);
    *support = GFP_READ | GFP_WRITE;

    return TRUE;
}


class BpgReader
{
private:
    bpg::Decoder dec;

    bool bDecReady;
    uint8_t *buf;

    void fullDecode();

public:
    uint8_t bpp;

    BpgReader(): bDecReady(false), buf(NULL), bpp(0) {}

    ~BpgReader() {
        if (buf)
            delete[] buf;
    }

    const bpg::ImageInfo& GetInfo() {
        return dec.GetInfo();
    }

    void DecodeFile (const char *s_file) {
        Benchmark bm("decode");
        dec.DecodeFile (s_file);
        bpp = GetInfo().GetBpp();
    }

    void PrepareDecode() {
        if (!bDecReady)
        {
            fullDecode();
            bDecReady = true;
        }
    }

    void CopyLine (int y, void *dst) {
        uint8_t *src = buf;
        size_t stride = GetInfo().width * bpp / 8;

        memcpy (dst, src + y * stride, stride);
    }
};


void BpgReader::fullDecode()
{
    Benchmark bm("full decode");
    /* Prepare decode */
    BPGDecoderOutputFormat fmt;
    const int8_t *shuffle;
    static const int8_t bpp24_shuffle[] = {0, 1, 2};
    static const int8_t bpp32_shuffle[] = {0, 1, 2, 3};
    switch (bpp)
    {
    case 24:
        fmt = BPG_OUTPUT_FORMAT_RGB24;
        shuffle = bpp24_shuffle;
        break;
    case 32:
        fmt = BPG_OUTPUT_FORMAT_RGBA32;
        shuffle = bpp32_shuffle;
        break;
    case 8:
    default:
        fmt = BPG_OUTPUT_FORMAT_GRAY8;
        shuffle = NULL;
        break;
    }

    dec.Start (fmt, shuffle, true);

    /* Allocate frame buffer */
    const bpg::ImageInfo &info2 = GetInfo();
    buf = new uint8_t [info2.width * info2.height * bpp / 8];

    class task: public LoopTask
    {
    private:
        bpg::Decoder *dec;
        uint8_t *buf;
        size_t stride;

    public:
        task (bpg::Decoder *dec, uint8_t *buf, size_t stride):
            dec(dec), buf(buf), stride(stride) {}

        virtual void loop (int begin, int end, int step) override {
            bpg::DecodeBuffer dbuf (dec, begin);
            uint8_t *dst = buf + begin * stride;

            for (int i = begin; i < end; i++)
            {
                dbuf.GetLine (dst);
                dst += stride;
            }
        }
    };

    /* Decode */
    LoopTaskSet set (gThreadPool, 0, info2.height, 1, MIN_LINES_PER_TASK);
    set.Dispatch<task> (&dec, buf, info2.width * bpp / 8);
}


EXTC void *API gfpLoadPictureInit (LPCSTR filename)
{
    Logi("%s: %s", __FUNCTION__, filename);
    try {
        gThreadPool->Start();

        BpgReader *dec = new BpgReader;
        dec->DecodeFile (filename);
        return dec;
    }
    catch (const exception &e) {
        Loge (e.what());
    }

    return NULL;
}


EXTC BOOL API gfpLoadPictureGetInfo (
    void *ptr,
    INT *pictype,
    INT *width,
    INT *height,
    INT *dpi,
    INT *bits_per_pixel,
    INT *bytes_per_line,
    BOOL *has_colormap,
    LPSTR label,
    INT label_max_size
)
{
    Logi("%s", __FUNCTION__);
    BpgReader &dec = *(BpgReader*)ptr;
    const bpg::ImageInfo &info = dec.GetInfo();

    *pictype = GFP_RGB;
    *width = info.width;
    *height = info.height;
    *dpi = 68;
    *bits_per_pixel = dec.bpp;
    *bytes_per_line = info.width * dec.bpp / 8;
    *has_colormap = FALSE;

    info.GetFormatDetail (label, label_max_size);
    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetLine (void *ptr, INT line, unsigned char *buffer)
{
    if (line==0)
        Logi("%s", __FUNCTION__);
    BpgReader &dec = *(BpgReader*)ptr;

    try {
        dec.PrepareDecode();
        dec.CopyLine (line, buffer);
    }
    catch (const exception &e) {
        Loge (e.what());
    }

    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetColormap (void *ptr, GFP_COLORMAP * cmap )
{
    return FALSE;
}


EXTC void API gfpLoadPictureExit( void * ptr )
{
    Logi("%s", __FUNCTION__);
    BpgReader *dec = (BpgReader*)ptr;
    delete dec;
}



class BpgWriter
{
private:
    std::shared_ptr<FILE> fp;

    int w, h;
    int bitsPerPixel;
    bpg::EncInputFormat inFmt;

    bpg::EncParam param;
    std::unique_ptr<bpg::Palette> pal;
    std::unique_ptr<bpg::EncImageBuffer> buf;
    std::unique_ptr<bpg::EncImage> img;

public:
    BpgWriter (const char s_file[], int w, int h, int bits_per_pixel):
        w(w), h(h), bitsPerPixel(bits_per_pixel), inFmt(bpg::INPUT_RGB24)
    {
        switch (bits_per_pixel)
        {
        case 8:
            inFmt = bpg::INPUT_GRAY8;
            break;
        case 24:
            inFmt = bpg::INPUT_RGB24;
            break;
        case 32:
            inFmt = bpg::INPUT_RGBA32;
            break;
        default:
            throw runtime_error ("invalid bits per pixel");
        }

        fp.reset (fopen (s_file, "wb"), fclose);
        if (!fp)
            throw runtime_error ("cannot open file");
    }


    void PutLine (int line, const void *data) {
        if (!buf)
            buf.reset ( new bpg::EncImageBuffer (w, h, inFmt) );

        buf->PutLine (line, data, pal.get());
    }

    void Write() {
        param.LoadConfig (ENC_CONFIG_FILE, bitsPerPixel);

        if (inFmt == bpg::INPUT_GRAY8)
            param->preferred_chroma_format = BPG_FORMAT_GRAY;

        img.reset (buf->CreateImage (param.cs, param.BitDepth));

        bpg::Encoder enc (param);
        enc.Encode (fp.get(), *img);
    }
};



// bits_per_pixel can be 1 to 8, 24, 32
EXTC BOOL API gfpSavePictureIsSupported( INT width, INT height, INT bits_per_pixel, BOOL has_colormap )
{
    Logi ("%s [%dx%d] %d bpp, colormap=%u\n", __FUNCTION__, width, height, bits_per_pixel, has_colormap);

    if (bits_per_pixel == 8 && !has_colormap)
        return TRUE;
    if (bits_per_pixel == 24)
        return TRUE;

    return FALSE;
}

EXTC void * API gfpSavePictureInit (
    LPCSTR filename,
    INT width, INT height,
    INT bits_per_pixel, INT dpi,
    INT * picture_type,
    LPSTR label, INT label_max_size )
{
    Logi ("%s [%dx%d] %d bpp\n", __FUNCTION__, width, height, bits_per_pixel);

    *picture_type = GFP_RGB;
    strncpy (label, FORMAT_NAME, label_max_size);

    try {
        BpgWriter *pwr = new BpgWriter (filename, width, height, bits_per_pixel);
        return pwr;
    }
    catch (const exception &e) {
        Loge (e.what());
        return NULL;
    }
}

EXTC BOOL API gfpSavePicturePutColormap (void *ptr, const GFP_COLORMAP *map)
{
    Logi ("%s map %p\n", __FUNCTION__, map);

    /* It's an undocumented interface
     * I just can't figure out how to support colormap
     */
    return FALSE;
}

EXTC BOOL API gfpSavePicturePutLine( void *ptr, INT line, const unsigned char *buffer )
{
    try {
        BpgWriter &wr = *(BpgWriter*)ptr;
        wr.PutLine (line, buffer);
        return TRUE;
    }
    catch (const exception &e) {
        Loge (e.what());
        return FALSE;
    }
}

EXTC void API gfpSavePictureExit( void * ptr )
{
    try {
        BpgWriter *pwr = (BpgWriter*)ptr;
        pwr->Write();
        delete pwr;
    }
    catch (const exception &e) {
        Loge (e.what());
    }

}
