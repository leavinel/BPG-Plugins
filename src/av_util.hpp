/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _AV_UTIL_HPP_
#define _AV_UTIL_HPP_


extern "C" {
#include "libavutil/pixdesc.h"
}

#ifdef AV_UTIL_SET
#define EXT
#else
#define EXT extern
#endif


/** Explicit libavutil.dll loader */
namespace avutil {

void init();
void deinit();

EXT const AVPixFmtDescriptor *(*av_pix_fmt_desc_get)(enum AVPixelFormat pix_fmt);
EXT int (*av_pix_fmt_count_planes)(enum AVPixelFormat pix_fmt);

}


#endif /* _AV_UTIL_HPP_ */
