/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _BENCHMARK_HPP_
#define _BENCHMARK_HPP_

#include <stdio.h>
#include <windows.h>
#include "log.h"


class Benchmark
{
private:
    const char *sMsg;
    DWORD begin;
public:
    Benchmark (const char *s_msg): sMsg(s_msg), begin(GetTickCount()) {}

    ~Benchmark() {
        Logi ("%s: %lu ms\n", sMsg, GetTickCount() - begin);
    }
};


#endif /* _BENCHMARK_HPP_ */
