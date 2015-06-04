/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "bpg_common.h"


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
