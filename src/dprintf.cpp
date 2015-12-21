/**
 * @file
 * Log API
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "log.h"


/**
 * Print error message
 */
void Loge (const char s_fmt[], ...)
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
void Logi (const char s_fmt[], ...)
{
    va_list ap;
    char buf[256];

    va_start (ap, s_fmt);
    vsnprintf (buf, sizeof(buf), s_fmt, ap);
    va_end (ap);

    strncat (buf, "\n", sizeof(buf));
    OutputDebugStringA (buf);
}
