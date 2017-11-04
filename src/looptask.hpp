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
 * A loop-based task which shall loop within index [begin, end)
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
class LoopTaskManager
{
private:
    winthread::mutex mtx;
    winthread::cond_var cv;
    ThreadPool &pool;
    int begin;
    int end;
    int step;
    int minIter;        ///< Minimum iterations per task

    volatile uint8_t waitCnt;

    int calcOptTaskCnt() const;
    int calcEndingIdx() const;
    void dispatchTasks (LoopTask* const ltasks[]);
    void threadProc (LoopTask *ltask, int begin, int end, int step);

public:
    LoopTaskManager (ThreadPool &pool):
        pool(pool), begin(0), end(0), step(0), minIter(0),
        waitCnt(0) {}

    void SetLoopRange (int begin, int end, int step = 1, int minIterPerTask = 1);

    /**
     * Create tasks, run and wait all task end
     * @tparam TASK Class which implements #LoopTask::loop()
     * @param args  Parameters to TASK initializer
     * @return Ending index of loops
     */
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
