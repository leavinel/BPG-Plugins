/*
 * Plugin example for XnView 1.65 or more
 *
 * When Xuser is built, put it in PluginsEx folder
 */

#include <stdio.h>
#include <windows.h>

#include "bpg_common.hpp"

using namespace std;


EXTC BOOL APIENTRY DllMain (HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH :
        dprintf ("Compiled at %s %s\n", __TIME__, __DATE__);
        BpgReader::InitClass();
        break;

    case DLL_PROCESS_DETACH :
        BpgReader::DeinitClass();
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


class BpgReaderPrivate: public BpgReader
{
public:
    uint8_t bpp;
    bool bStarted;

    BpgReaderPrivate(): bpp(0), bStarted(false) {}

    void StartDecode() {
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

        GetDecoder().Start (fmt, shuffle, true);
        bStarted = true;
    }
};


EXTC void *API gfpLoadPictureInit (LPCSTR filename)
{
    try {
        BpgReaderPrivate *preader = new BpgReaderPrivate();
        preader->LoadFromFile (filename);
        return preader;
    }
    catch (const exception &e) {
        pr_err (e.what());
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
    BpgReaderPrivate &reader = *(BpgReaderPrivate*)ptr;
    BpgDecoder &decoder = reader.GetDecoder();
    const BpgImageInfo2 &info = decoder.GetInfo();
    reader.bpp = info.GetBpp();

    *pictype = GFP_RGB;
    *width = info.width;
    *height = info.height;
    *dpi = 68;
    *bits_per_pixel = reader.bpp;
    *bytes_per_line = info.width * reader.bpp / 8;
    *has_colormap = FALSE;

    info.GetFormatDetail (label, label_max_size);
    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetLine (void *ptr, INT line, unsigned char *buffer)
{
    BpgReaderPrivate &reader = *(BpgReaderPrivate*)ptr;

    try {
        if (!reader.bStarted)
            reader.StartDecode();

        BpgDecoder &decoder = reader.GetDecoder();
        decoder.GetLine (line, buffer);
    }
    catch (const exception &e) {
        pr_err (e.what());
    }

    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetColormap (void *ptr, GFP_COLORMAP * cmap )
{
    return FALSE;
}


EXTC void API gfpLoadPictureExit( void * ptr )
{
    BpgReaderPrivate *preader = (BpgReaderPrivate*)ptr;
    delete preader;
}


// bits_per_pixel can be 1 to 8, 24, 32
EXTC BOOL API gfpSavePictureIsSupported( INT width, INT height, INT bits_per_pixel, BOOL has_colormap )
{
    dprintf ("%s [%dx%d] %d bpp, colormap=%u\n", __FUNCTION__, width, height, bits_per_pixel, has_colormap);
    if (bits_per_pixel == 24)
        return TRUE;

    if (bits_per_pixel == 8 && !has_colormap)
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
    dprintf ("%s [%dx%d] %d bpp\n", __FUNCTION__, width, height, bits_per_pixel);
    BpgWriter *pfile;

    try {
        pfile = new BpgWriter (filename, width, height, bits_per_pixel);
    }
    catch (const exception &e) {
        pr_err (e.what());
        return NULL;
    }

    *picture_type = GFP_RGB;

    strncpy (label, FORMAT_NAME, label_max_size);
    return pfile;
}

EXTC BOOL API gfpSavePicturePutColormap (void *ptr, const GFP_COLORMAP *map)
{
    return FALSE;
}

EXTC BOOL API gfpSavePicturePutLine( void *ptr, INT line, const unsigned char *buffer )
{
    BpgWriter &file = *(BpgWriter*)ptr;

    try {
        file.PutLine (line, buffer);
    }
    catch (const exception &e) {
        pr_err (e.what());
        return FALSE;
    }

    return TRUE;
}

EXTC void API gfpSavePictureExit( void * ptr )
{
    BpgWriter *pfile = (BpgWriter*)ptr;
    BpgWriter &file = *pfile;

    try {
        file.Write();
        delete pfile;
    }
    catch (const exception &e) {
        pr_err (e.what());
    }

}
