/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#include <stdio.h>
#include <windows.h>


class Benchmark
{
private:
    const char *sMsg;
    DWORD begin;
public:
    Benchmark (const char *s_msg): sMsg(s_msg), begin(GetTickCount()) {}

    ~Benchmark() {
        printf ("%s: %lu ms\n", sMsg, GetTickCount() - begin);
    }
};


#endif /* _BENCHMARK_H_ */
