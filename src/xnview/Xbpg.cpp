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
#include "av_util.hpp"

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
        bpg::gThreadPool = new ThreadPool;
        avutil::init();
        break;

    case DLL_PROCESS_DETACH :
        avutil::deinit();
        delete bpg::gThreadPool;
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


struct BpgReader
{
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

    BpgReader &r = *(BpgReader*)ptr;

    try {
        if (!r.frame)
            r.dec.ConvertToFrame (r.frame);

        r.frame.GetLine (line, buffer);
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



struct BpgWriter
{
    bpg::pFILE fp;
    bpg::EncParam param;
    bpg::Frame frame;

    BpgWriter(): fp(nullptr, fclose) {}
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
    BpgWriter *pw = NULL;
    enum AVPixelFormat fmt;
    const char *s_prefix;

    Logi ("%s [%dx%d] %d bpp\n", __FUNCTION__, width, height, bits_per_pixel);

    *picture_type = GFP_RGB;
    strncpy (label, FORMAT_NAME, label_max_size);

    try {
        pw = new BpgWriter;
        BpgWriter &w = *pw;

        switch (bits_per_pixel)
        {
            case 8:
                fmt = AV_PIX_FMT_GRAY8;
                w.param->preferred_chroma_format = BPG_FORMAT_GRAY;
                s_prefix = "8:";
                break;
            case 24:
                fmt = AV_PIX_FMT_RGB24;
                s_prefix = "24:";
                break;
            case 32:
                fmt = AV_PIX_FMT_RGBA;
                s_prefix = "32:";
                break;
            default:
                throw runtime_error ("invalid BPP");
        }

        /* Load arguments from INI file */
        {
            bpg::IniFile f_ini;
            std::string s_opts;

            f_ini.Open (ENC_CONFIG_FILE);
            f_ini.GetLineByPrefix (s_opts, s_prefix);
            w.param.Parse (s_opts.c_str());
        }

        w.frame.AllocByFormat (width, height, fmt);

        /* Open file to write */
        w.fp = bpg::pFILE (fopen (filename, "wb"), fclose);
        if (!w.fp)
            throw runtime_error (string("Cannot open file ") + filename);
    }
    catch (const exception &e) {
        Loge (e.what());
        if (pw)
            delete pw;
        return NULL;
    }

    return pw;
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
    BpgWriter &w = *(BpgWriter*)ptr;

    try {
        w.frame.SetLine (line, buffer);
        return TRUE;
    }
    catch (const exception &e) {
        Loge (e.what());
        return FALSE;
    }
}

EXTC void API gfpSavePictureExit( void * ptr )
{
    BpgWriter *pwr = (BpgWriter*)ptr;
    BpgWriter &w = *pwr;

    try {
        bpg::Encoder enc;
        enc.Encode (w.fp.get(), w.param, w.frame);
    }
    catch (const exception &e) {
        Loge (e.what());
    }

    delete pwr;
}
