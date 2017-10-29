/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */


#include <windows.h>

#define AV_UTIL_SET
#include "av_util.hpp"

#define DLL_NAME    "avutil-55.dll"

using namespace avutil;


static HINSTANCE h_dll;


void avutil::init()
{
    h_dll = LoadLibraryA (DLL_NAME);
    if (!h_dll)
        return;

#define LOAD_FUNC(name) \
    name = (typeof(name)) GetProcAddress (h_dll, #name)
    LOAD_FUNC (av_pix_fmt_desc_get);
    LOAD_FUNC (av_pix_fmt_count_planes);
}


void avutil::deinit()
{
    if (!h_dll)
        return;

    FreeLibrary (h_dll);
}
