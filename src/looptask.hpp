/**
 * @file
 * Loop-based task parallelization
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _LOOPTASK_HPP_
#define _LOOPTASK_HPP_


#include "threadpool.hpp"


/**
 * A loop-based task
 */
struct LoopTask
{
    virtual ~LoopTask(){}
    /**
     * Loop context, which ranges from [begin, end)
     */
    virtual void loop (int begin, int end, int step) = 0;
};


/**
 * A loop-based task parallelization dispatcher
 */
class LoopTaskSet
{
private:
    class innerTask;

    winthread::mutex mtx;
    winthread::cond_var cv;

    ThreadPool *pool;
    int begin;
    int end;
    int step;
    int minIter;        ///< Minimum interations per task

    volatile uint8_t waitCnt;

    void reportDone (innerTask *itask);

    int calcOptTaskCnt() const;
    int calcEndingIdx() const;
    void dispatchTasks (LoopTask *ltasks[]);

public:
    LoopTaskSet (ThreadPool *pool, int begin, int end, int step = 1, int minIterPerTask = 1):
        pool(pool), begin(begin), end(end), step(step), minIter(minIterPerTask),
        waitCnt(0) {}

    template <class TASK, typename... Args>
    int Dispatch (Args&&... args) {
        uint8_t taskCnt = calcOptTaskCnt();

        if (taskCnt == 1) // Single-thread
        {
            TASK ltask (args...);
            ltask.loop (begin, end, step);
        }
        else // Multi-thread
        {
            LoopTask *ltasks[taskCnt];

            for (size_t i = 0; i < taskCnt; i++)
                ltasks[i] = new TASK (args...);

            dispatchTasks (ltasks);

            for (size_t i = 0; i < taskCnt; i++)
                delete ltasks[i];
        }

        return calcEndingIdx();
    }
};


#endif /* _LOOPTASK_HPP_ */
