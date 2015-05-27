/*
 * Plugin example for XnView 1.65 or more
 *
 * When Xuser is built, put it in PluginsEx folder
 */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "Xbpg.h"

#define FORMAT_NAME     "BPG Image"
#define FORMAT_EXT      "bpg"

using namespace std;

#define EXTC extern "C"

/**
 * Print error message
 */
void pr_err (const char s_fmt[], ...)
{
    va_list ap;
    char buf[256];

    va_start (ap, s_fmt);
    vsnprintf (buf, sizeof(buf), s_fmt, ap);
    va_end (ap);

    MessageBoxA (NULL, buf, "Error", MB_OK);
}


/**
 * Print debug message to debugger
 */
void dprintf (const char s_fmt[], ...)
{
    va_list ap;
    char buf[256];

    va_start (ap, s_fmt);
    vsnprintf (buf, sizeof(buf), s_fmt, ap);
    va_end (ap);

    OutputDebugStringA (buf);
}


EXTC BOOL APIENTRY DllMain (HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH :
        dprintf ("Compiled at %s %s\n", __TIME__, __DATE__);
        break;

    case DLL_PROCESS_DETACH :
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


EXTC void *API gfpLoadPictureInit (LPCSTR filename)
{
    try {
        BpgReader *pfile = new BpgReader (filename);
        return pfile;
    }
    catch (const exception &e) {
        pr_err (e.what());
    }

    return NULL;
}


static void format_label (char *label, int sz, int fmt, int cs)
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

    snprintf (label, sz, "BPG %s %s", s_fmt[fmt], s_cs[cs]);
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
    BpgReader &file = *(BpgReader*)ptr;

    BPGImageInfo &info = file.info;
    *pictype = GFP_RGB;
    *width = info.width;
    *height = info.height;
    *dpi = 68;
    *bits_per_pixel = file.bitsPerPixel;
    *bytes_per_line = file.linesz;
    *has_colormap = FALSE;

    format_label (label, label_max_size, info.format, info.color_space);
    return TRUE;
}


EXTC BOOL API gfpLoadPictureGetLine (void *ptr, INT line, unsigned char *buffer)
{
    BpgReader &file = *(BpgReader*)ptr;

    try {
        file.GetLine (line, buffer);
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
    BpgReader *pfile = (BpgReader*)ptr;
    delete pfile;
}


// bits_per_pixel can be 1 to 8, 24, 32
EXTC BOOL API gfpSavePictureIsSupported( INT width, INT height, INT bits_per_pixel, BOOL has_colormap )
{
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
