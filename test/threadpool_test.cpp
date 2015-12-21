/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <windows.h>
#include <stdio.h>
#include "threadpool.hpp"

using namespace std;
//using namespace pthread;
using namespace winthread;


class Task: public task
{
public:
    int const idx;
    mutex &mtx;

    Task (int idx, mutex &mtx): idx(idx), mtx(mtx) {
    }

    virtual ~Task(){}

    virtual void run() override {
        {
            lock_guard _l(mtx);
            printf ("task %u start\n", idx);
        }

        for (int i = 0; i < 3; i++)
        {
            Sleep(1000);
            {
                lock_guard _l(mtx);
                printf ("task %u sleep %u\n", idx, i);
            }
        }

        {
            lock_guard _l(mtx);
            printf ("task %u finish\n", idx);
        }
    }
};

#define TASK_NUM    10

int main()
{
    ThreadPool pool(4);
    Task *tasks[TASK_NUM];
    mutex mtx;
#if 1
    for (int i = 0; i < TASK_NUM; i++)
        tasks[i] = new Task (i, mtx);

    for (int i = 0; i < TASK_NUM; i++)
        pool.EnqueueTask (tasks[i]);
#endif
    pool.Start();
    puts ("Waiting ThreadPool...");
    pool.Join();
    puts ("ThreadPool done");

    return 0;
}

