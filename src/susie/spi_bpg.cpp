/**
 * @file
 * BPG plugin for Susie
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <string.h>
#include <stddef.h>
#include <exception>

extern "C" {
#include "spi00in.h"
}

#include "bpg_common.h"


#define NELEM(ary)      ((size_t)(sizeof(ary)/sizeof(ary[0])))
#define ALIGN(x,n)      ((((x) + ((n)-1)) / (n)) * (n))

using namespace std;


EXTC int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen)
{
    static const char *pluginfo[] = {
        "00IN",             // Plugin version
        MODULE_NAME,        // Plugin name
        "*." FORMAT_EXT,    // File extension
        FORMAT_NAME,        // File format
    };

    if (infono < 0 || infono >= NELEM(pluginfo))
        return 0;

    lstrcpyn(buf, pluginfo[infono], buflen);

    return lstrlen(buf);
}

EXTC int __stdcall IsSupported(LPSTR filename, DWORD dw)
{
    char *data;
    char buff[BpgImageInfo2::HEADER_MAGIC_SIZE];
    DWORD ReadBytes;

    if ((dw & 0xFFFF0000) == 0) {
    /* dw is a file handle */
        if (!ReadFile((HANDLE)dw, buff, sizeof(buff), &ReadBytes, NULL)) {
            return 0;
        }
        data = buff;
    } else {
    /* dw is a buffer pointer */
        data = (char*)dw;
    }

    if (BpgImageInfo2::CheckHeader (data, sizeof(buff)))
        return 1;

    return 0;
}

EXTC int __stdcall GetPictureInfo
(LPSTR buf, long len, unsigned int flag, struct PictureInfo *lpInfo)
{
    int ret = SPI_OTHER_ERROR;
    BpgReader reader;

    try {
        if ((flag & 7) == 0) {
        /* buf is the filename */
            reader.LoadFromFile (buf);
        } else {
        /* buf is the pointer to buffer */
            reader.LoadFromBuffer (buf, len);
        }

        const BpgImageInfo2 &info = reader.GetDecoder().GetInfo();
        lpInfo->left        = 0;
        lpInfo->top         = 0;
        lpInfo->width       = info.width;
        lpInfo->height      = info.height;
        lpInfo->x_density   = 0;
        lpInfo->y_density   = 0;
        lpInfo->colorDepth  = info.GetBpp();
        lpInfo->hInfo       = NULL;

        ret = SPI_ALL_RIGHT;
    } catch (const exception &e) {
        pr_err (e.what());
    }

    return ret;
}

EXTC int __stdcall GetPicture(
        LPSTR buf, long len, unsigned int flag,
        HANDLE *pHBInfo, HANDLE *pHBm,
        SPI_PROGRESS lpPrgressCallback, long lData)
{
    int ret = SPI_OTHER_ERROR;
    BpgReader reader;

    if (lpPrgressCallback)
        lpPrgressCallback (0, 1, lData);

    try {
        if ((flag & 7) == 0) {
        /* buf is the filename */
            reader.LoadFromFile (buf);
        } else {
        /* buf is the pointer to buffer */
            reader.LoadFromBuffer (buf, len);
        }

        BpgDecoder &decoder = reader.GetDecoder();
        const BpgImageInfo2 &info = decoder.GetInfo();
        int bpp = info.GetBpp();

        if (lpPrgressCallback)
            lpPrgressCallback (1, 2, lData); // 50%

        /* Allocate BITMAPINFO & image buffer */
        size_t infosz;

        if (8 == bpp)
            infosz = offsetof(BITMAPINFO, bmiColors[256]);
        else
            infosz = offsetof(BITMAPINFO, bmiColors);

        size_t linesz = ALIGN (info.width * bpp / 8, 4);
        size_t imgsz = linesz * info.height;
        BITMAPINFO *pbmpinfo;
        void *buf;

        *pHBInfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, infosz);
        if (!pHBInfo)
            goto fail_alloc_info;

        *pHBm = LocalAlloc (LMEM_MOVEABLE, imgsz);
        if (!pHBm)
            goto fail_alloc_img;

        pbmpinfo = (BITMAPINFO*) LocalLock (*pHBInfo);
        if (!pbmpinfo)
            goto fail_lock_info;

        buf = LocalLock (*pHBm);
        if (!buf)
            goto fail_lock_img;

        {
            BITMAPINFOHEADER &hdr = pbmpinfo->bmiHeader;
            hdr.biSize          = sizeof(BITMAPINFOHEADER);
            hdr.biWidth         = info.width;
            hdr.biHeight        = info.height;
            hdr.biPlanes        = 1;
            hdr.biBitCount      = bpp;
            hdr.biCompression   = BI_RGB;

            if (8 == bpp)
            {
                for (int i = 0; i < 256; i++)
                {
                    RGBQUAD &rgb = pbmpinfo->bmiColors[i];
                    rgb.rgbRed   = i;
                    rgb.rgbGreen = i;
                    rgb.rgbBlue  = i;
                }
            }

            BPGDecoderOutputFormat fmt;
            const int8_t *shuffle;
            static const int8_t bpp24_shuffle[] = {2, 1, 0};
            static const int8_t bpp32_shuffle[] = {2, 1, 0, 3};
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

            decoder.Start (fmt, shuffle);

            uint8_t *bufline = (uint8_t*)buf + linesz * info.height;
            for (size_t y = 0; y < info.height; y++)
            {
                bufline -= linesz;
                decoder.GetLine (y, bufline);

                if (lpPrgressCallback)
                    lpPrgressCallback (info.height + y, info.height*2, lData);
            }
        }

        if (lpPrgressCallback)
            lpPrgressCallback (1, 1, lData);

        /* Clean up */
        LocalUnlock (*pHBm);
        LocalUnlock (*pHBInfo);

        ret = SPI_ALL_RIGHT;
        return ret;

fail_lock_img:
        LocalUnlock (*pHBInfo);
fail_lock_info:
        LocalFree (*pHBm);
        *pHBm = NULL;
fail_alloc_img:
        LocalFree (*pHBInfo);
        *pHBInfo = NULL;
fail_alloc_info:
        ;
    }
    catch (const exception &e) {
        pr_err (e.what());
    }

    return ret;
}

EXTC int __stdcall GetPreview(
    LPSTR buf, long len, unsigned int flag,
    HANDLE *pHBInfo, HANDLE *pHBm,
    SPI_PROGRESS lpPrgressCallback, long lData)
{
    return SPI_NO_FUNCTION;
}


EXTC BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason, LPVOID lpReserved)
{
    switch (ul_reason)
    {
    case DLL_PROCESS_ATTACH:
        dprintf ("Compiled at %s %s\n", __TIME__, __DATE__);
        BpgReader::InitClass();
        break;

    case DLL_PROCESS_DETACH:
        BpgReader::DeinitClass();
        break;

    default:
        break;
    }

    return TRUE;
}
