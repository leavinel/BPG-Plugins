/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

extern "C" {
#include <stdio.h>
#include <windows.h>
#include "ImagPlug.h"
}

#include "log.h"

#include <exception>
#include "av_util.hpp"

#define BPG_COMMON_SET
#include "bpg_common.hpp"

#define _VERSION_NUMBER(a,b,c,d)     ((a<<24) | (b<<16) | (c<<8) | d)
#define VERSION_NUMBER(abcd)      _VERSION_NUMBER (abcd)

#define __UNICODE_TEXT(x) L##x
#define UNICODE_TEXT(x) __UNICODE_TEXT(x)


using namespace std;


static PALETTEENTRY *get_gray_palette()
{
    static bool b_gray_palette_inited = false;
    static PALETTEENTRY gray_palette[256];

    if (!b_gray_palette_inited)
    {
        /* Init grayscale palette */
        for (int i = 0; i < 256; i++)
        {
            gray_palette[i].peRed = i;
            gray_palette[i].peGreen = i;
            gray_palette[i].peBlue = i;
            gray_palette[i].peFlags = 0;
        }

        b_gray_palette_inited = true;
    }

    return gray_palette;
}


static BOOL IMAGINEAPI checkFile (IMAGINEPLUGINFILEINFOTABLE *fileInfoTable,IMAGINELOADPARAM *loadParam,int flags)
{
    return bpg::ImageInfo::CheckHeader (loadParam->buffer, loadParam->length);
}


static LPIMAGINEBITMAP IMAGINEAPI loadFile(IMAGINEPLUGINFILEINFOTABLE *fileInfoTable,IMAGINELOADPARAM *loadParam,int flags)
{
    const IMAGINEPLUGININTERFACE *iface = fileInfoTable->iface;
    if (!iface)
        return NULL;

    try {
        enum AVPixelFormat dst_fmt;
        uint8_t *dst;
        int dst_stride;
        bpg::Decoder dec;

        if (flags & IMAGINELOADPARAM_GETINFO)
            dec.DecodeBuffer (loadParam->buffer, loadParam->length, bpg::Decoder::OPT_HEADER_ONLY);
        else
            dec.DecodeBuffer (loadParam->buffer, loadParam->length);

        const bpg::ImageInfo &info = dec.GetInfo();
        uint8_t bpp = info.GetBpp();
        LPIMAGINEBITMAP bitmap = iface->lpVtbl->Create (info.width, info.height, bpp, flags);
        if (!bitmap)
        {
            loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
            return NULL;
        }

        if (flags & IMAGINELOADPARAM_GETINFO)
            return bitmap;

        /* If grayscale, set palette */
        switch (bpp)
        {
        case 8:
            iface->lpVtbl->SetPalette (bitmap, get_gray_palette());
            dst_fmt = AV_PIX_FMT_GRAY8;
            break;
        case 24:
            dst_fmt = AV_PIX_FMT_BGR24;
            break;
        case 32:
            dst_fmt = AV_PIX_FMT_BGRA;
            break;
        default:
            throw runtime_error ("invalid bpp");
        }

        /* Bitmap is upside-down */
        size_t linesz = iface->lpVtbl->GetWidthBytes (bitmap);
        dst = (uint8_t*) iface->lpVtbl->GetBits (bitmap);
        dst += linesz * (info.height - 1);
        dst_stride = -linesz;
        dec.Convert (dst_fmt, dst, dst_stride);
        return bitmap;
    }
    catch (const exception &e) {
        Loge (e.what());
        loadParam->errorCode = IMAGINEERROR_READERROR;
        return NULL;
    }
}


static BOOL IMAGINEAPI saveFile(IMAGINEPLUGINFILEINFOTABLE *fileInfoTable,LPIMAGINEBITMAP bitmap,IMAGINESAVEPARAM *saveParam,int flags)
{
    return FALSE;
}


static BOOL IMAGINEAPI registerProcA(const IMAGINEPLUGININTERFACE *iface)
{
    static const IMAGINEFILEINFOITEM fileInfoItemA=
    {
        checkFile,
        loadFile,
        saveFile,
        FORMAT_NAME,
        FORMAT_EXT "\0",
    };
    return (NULL != iface->lpVtbl->RegisterFileType(&fileInfoItemA));
}


static BOOL IMAGINEAPI registerProcW(const IMAGINEPLUGININTERFACE *iface)
{
    static const IMAGINEFILEINFOITEM fileInfoItemW=
    {
        checkFile,
        loadFile,
        saveFile,
        (LPCTSTR) UNICODE_TEXT(FORMAT_NAME),
        (LPCTSTR) UNICODE_TEXT(FORMAT_EXT "\0"),
    };
    return (NULL != iface->lpVtbl->RegisterFileType(&fileInfoItemW));
}


EXTC BOOL CALLBACK APIENTRY DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved)
{
    switch (dwReason)
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


EXTC BOOL IMAGINEAPI ImaginePluginGetInfoA(IMAGINEPLUGININFOA *dest)
{
    static const IMAGINEPLUGININFOA pluginInfoA=
    {
        sizeof(pluginInfoA),
        registerProcA,
        VERSION_NUMBER (MODULE_VER),
        MODULE_NAME,
        IMAGINEPLUGININTERFACE_VERSION,
    };
    *dest=pluginInfoA;
    return TRUE;
}


EXTC BOOL IMAGINEAPI ImaginePluginGetInfoW(IMAGINEPLUGININFOW *dest)
{
    static const IMAGINEPLUGININFOW pluginInfoW=
    {
        sizeof(pluginInfoW),
        registerProcW,
        VERSION_NUMBER (MODULE_VER),
        UNICODE_TEXT (MODULE_NAME),
        IMAGINEPLUGININTERFACE_VERSION,
    };
    *dest=pluginInfoW;
    return TRUE;
}
