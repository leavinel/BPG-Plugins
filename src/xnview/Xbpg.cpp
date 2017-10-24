/*
 * Plugin example for XnView 1.65 or more
 *
 * When Xuser is built, put it in PluginsEx folder
 */

extern "C" {
#include <stdio.h>
#include <windows.h>
}

#include "log.h"

#define BPG_COMMON_SET
#include "bpg_common.hpp"


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
public:
    bpg::Decoder dec;
    bpg::Frame frame;
};



EXTC void *API gfpLoadPictureInit (LPCSTR filename)
{
    Logi("%s: %s", __FUNCTION__, filename);
    try {
        BpgReader *r = new BpgReader;
        bpg::Decoder &dec = r->dec;

        dec.DecodeFile (filename);
        return r;
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
    BpgReader *r = (BpgReader*)ptr;
    bpg::Decoder &dec = r->dec;
    const bpg::ImageInfo &info = dec.GetInfo();
    uint8_t bpp = info.GetBpp();

    *pictype = GFP_RGB;
    *width = info.width;
    *height = info.height;
    *dpi = 68;
    *bits_per_pixel = bpp;
    *bytes_per_line = info.width * bpp / 8;
    *has_colormap = FALSE;

    info.GetFormatDetail (label, label_max_size);
    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetLine (void *ptr, INT line, unsigned char *buffer)
{
    if (line==0)
        Logi("%s", __FUNCTION__);

    BpgReader *r = (BpgReader*)ptr;
    bpg::Decoder &dec = r->dec;
    bpg::Frame &frame = r->frame;

    try {
        if (!frame.isReady())
            dec.ConvertToFrame (frame, *gThreadPool);

        frame.GetLine (line, buffer);
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
    shared_ptr<FILE> fp;

    int w, h;
    int bitsPerPixel;
    bpg::EncInputFormat inFmt;

    bpg::EncParam param;
    unique_ptr<bpg::Palette> pal;
    unique_ptr<bpg::EncImageBuffer> buf;
    unique_ptr<bpg::EncImage> img;

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
