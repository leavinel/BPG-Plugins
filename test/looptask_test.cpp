/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdio.h>
#include <windows.h>

#include "looptask.hpp"

using namespace winthread;

static mutex mtx;


class Task: public LoopTask
{
private:
    int a, b, c;

    void print (int i) {
        lock_guard _l(mtx);
        printf ("task %p, %d\n", this, i);
    }

public:
    Task (int a, int b, int c): a(a), b(b), c(c) {
    }

    virtual ~Task(){}

    virtual void loop (int begin, int end, int step) override {

        for (int i = begin; i < end; i+=step)
        {
            print(i);
            Sleep (1000);
        }
    }
};



int main (void)
{
    ThreadPool pool;
    pool.Start();

    LoopTaskManager set (&pool, -10, 10, 2);
    int last = set.Dispatch<Task> (2,3,4);

    printf ("last index: %d\n", last);

    pool.Join();

    return 0;
}

