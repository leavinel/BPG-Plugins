/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <exception>
#include <windows.h>
#include "bpg_common.h"

extern "C" {
#include "ImagPlug.h"
}

#define VERSION_NUMBER1 0
#define VERSION_NUMBER2 0
#define VERSION_NUMBER3 0
#define VERSION_NUMBER4 1
#define VERSION_NUMBER (VERSION_NUMBER1<<24)|(VERSION_NUMBER2<<16)|(VERSION_NUMBER3<<8)|(VERSION_NUMBER4<<0)

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
    return BpgImageInfo2::CheckHeader (loadParam->buffer, loadParam->length);
}


static LPIMAGINEBITMAP IMAGINEAPI loadFile(IMAGINEPLUGINFILEINFOTABLE *fileInfoTable,IMAGINELOADPARAM *loadParam,int flags)
{
    const IMAGINEPLUGININTERFACE *iface = fileInfoTable->iface;
    if (!iface)
        return NULL;

    try {
        if (flags & IMAGINELOADPARAM_GETINFO)
        {
            BpgImageInfo2 info (loadParam->buffer, loadParam->length);
            uint8_t bpp = info.GetBpp();
            dprintf ("%s [%ux%u] @ %u bpp flags%X\n", __FUNCTION__, info.width, info.height, bpp, flags);
            LPIMAGINEBITMAP bitmap = iface->lpVtbl->Create (info.width, info.height, bpp, flags);
            if (!bitmap)
            {
                loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
                return NULL;
            }

            return bitmap;
        }

        BpgReader reader;
        reader.LoadFromBuffer (loadParam->buffer, loadParam->length);

        const BPGImageInfo &info = reader.info;
        dprintf ("%s [%ux%u] @ %u bpp flags%X\n", __FUNCTION__, info.width, info.height, reader.bitsPerPixel, flags);
        LPIMAGINEBITMAP bitmap = iface->lpVtbl->Create (info.width, info.height, reader.bitsPerPixel, flags);
        if (!bitmap)
        {
            loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
            return NULL;
        }

        /* If grayscale, set palette */
        if (reader.bitsPerPixel == 8)
            iface->lpVtbl->SetPalette (bitmap, get_gray_palette());

        IMAGINECALLBACKPARAM cb;
        cb.dib = bitmap;
        cb.param = loadParam->callback.param;
        cb.overall = info.height - 1;

        for (uint32_t y = 0; y < info.height; y++)
        {
            uint8_t *dst = (uint8_t*)iface->lpVtbl->GetLineBits (bitmap, y);
            static const uint8_t rgba_offset[] = {2,1,0,3};
            reader.GetLine (y, dst, rgba_offset);

            cb.current = y;
            if ( (flags&IMAGINELOADPARAM_CALLBACK) && (!loadParam->callback.proc(&cb)))
            {
                loadParam->errorCode = IMAGINEERROR_ABORTED;
                break;
            }
        }

        return bitmap;
    }
    catch (const exception &e) {
        pr_err (e.what());
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
        FORMAT_EXT,
    };
    return (BOOL)iface->lpVtbl->RegisterFileType(&fileInfoItemA);
}


static BOOL IMAGINEAPI registerProcW(const IMAGINEPLUGININTERFACE *iface)
{
    static const IMAGINEFILEINFOITEM fileInfoItemW=
    {
        checkFile,
        loadFile,
        saveFile,
        (LPCTSTR) UNICODE_TEXT(FORMAT_NAME),
        (LPCTSTR) UNICODE_TEXT(FORMAT_EXT),
    };
    return (BOOL)iface->lpVtbl->RegisterFileType(&fileInfoItemW);
}


EXTC BOOL CALLBACK APIENTRY DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID lpvReserved)
{
    switch (dwReason)
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


EXTC BOOL IMAGINEAPI ImaginePluginGetInfoA(IMAGINEPLUGININFOA *dest)
{
    static const IMAGINEPLUGININFOA pluginInfoA=
    {
        sizeof(pluginInfoA),
        registerProcA,
        VERSION_NUMBER,
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
        VERSION_NUMBER,
        UNICODE_TEXT (MODULE_NAME),
        IMAGINEPLUGININTERFACE_VERSION,
    };
    *dest=pluginInfoW;
    return TRUE;
}
