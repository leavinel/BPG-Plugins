/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdlib.h>
#include "bpg_common.h"
#include "benchmark.h"


int main (int argc, char *argv[])
{
    BpgReader reader;

    {
        Benchmark bm ("decode");
        reader.LoadFromFile (argv[1]);
    }

    BpgDecoder &decoder = reader.GetDecoder();
    const BpgImageInfo2 &info = decoder.GetInfo();
    size_t sz = info.width * info.GetBpp();
    void *buf = malloc (sz);

    static const int8_t shuffle[] = {0, 1, 2, 3};

    decoder.Start(BPG_OUTPUT_FORMAT_RGB24, shuffle);

    int h = info.height;
    {
        Benchmark bm ("YUV=>RGB");
        for (int i = 0; i < h; i++)
            decoder.GetLine (i, buf);
    }

    free (buf);
    return 0;
}
