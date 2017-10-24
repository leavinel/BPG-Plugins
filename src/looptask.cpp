/**
 * @file
* Loop-based task parallelization
  *
 * @author Leav Wu (leavinel@gmail.com)
 */


#include "looptask.hpp"

using namespace std;
using namespace winthread;


/**
 * LoopTask inner task wrapper class
 */
class LoopTaskManager::innerTask: public task
{
private:
    LoopTaskManager *set;
    LoopTask *ltask;
    int begin;
    int end;
    int step;

public:
    innerTask (LoopTaskManager *set, LoopTask *ltask, int begin, int end, int step):
        set(set), ltask(ltask), begin(begin), end(end), step(step) {}

    virtual ~innerTask(){}

    virtual void run() override {
        /* Execute loop, then report done */
        ltask->loop (begin, end, step);
        set->reportDone (this);
    }
};


void LoopTaskManager::SetLoopRange (int begin, int end, int step, int minIterPerTask)
{
    this->begin = begin;
    this->end   = end;
    this->step  = step;
    this->minIter = minIterPerTask;
}


/**
 * For inner tasks to report that the task done
 */
void LoopTaskManager::reportDone (innerTask *itask)
{
    lock_guard _l(mtx);

    if (--waitCnt == 0)
        cv.notify_one();
}


/**
 * Calculate optimized task count for parallel execution
 */
int LoopTaskManager::calcOptTaskCnt() const
{
    uint8_t taskCnt = 1;

    /* Get basic task count by number of processor cores */
    taskCnt = pool.GetNumOfProc();

    if (taskCnt == 1) // Only 1 core available
        return 1;

    /* Limit task count by min. iterations per task */
    int iterCnt = (end - begin) / step; // Total iterations
    int maxTaskCnt = iterCnt / minIter;

    if (maxTaskCnt == 0)
        maxTaskCnt = 1;

    if (taskCnt > maxTaskCnt)
        taskCnt = maxTaskCnt;

    return taskCnt;
}


/**
 * Calculate the ending index of the loop
 */
int LoopTaskManager::calcEndingIdx() const
{
    return begin + ((end - begin + step-1) / step) * step;
}


/**
 * Wait all inner task is done
 */
void LoopTaskManager::dispatchTasks (LoopTask *ltasks[])
{
    size_t procCnt = pool.GetNumOfProc();
    innerTask *itasks[procCnt];

    int loopCnt = (end - begin) / step;
    int loopPerProc = loopCnt / procCnt;
    int begin2 = begin, end2;

    for (size_t i = 0; i < procCnt; i++)
    {
        if (i == procCnt-1) // Last task
            end2 = end;
        else
            end2 = begin + loopPerProc * step;

        itasks[i] = new innerTask (this, ltasks[i], begin2, end2, step);
        begin2 = end2;
    }

    waitCnt = procCnt;

    /* Wait until all tasks are done */
    {
        lock_guard _l(mtx);

        for (size_t i = 0; i < procCnt; i++)
            pool.EnqueueTask (itasks[i]);

        cv.wait (mtx);
    }

    for (size_t i = 0; i < procCnt; i++)
        delete itasks[i];
}
