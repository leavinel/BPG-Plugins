/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _THREADPOOL_HPP_
#define _THREADPOOL_HPP_


#include <stdio.h>

#include <queue>
#include <memory>

#include "winthread.hpp"


class ThreadPool;


/**
 * Thread which waits and runs the task queue
 */
class WorkerThread: public winthread::thread
{
private:
    ThreadPool &pool;

public:
    WorkerThread (ThreadPool &pool): pool(pool) {}
    virtual ~WorkerThread() override {}

    void start();
    void join();

    /* pthread::task */
    virtual void run() override;
};


/**
 * WorkerThread management pool
 */
class ThreadPool
{
    friend class WorkerThread;
private:
    uint8_t numOfProc;
    bool bStarted;
    winthread::mutex taskMtx;
    winthread::cond_var taskCv;
    std::queue<winthread::task*> tasks;
    std::vector<std::shared_ptr<WorkerThread>> threads;

public:
    enum { AUTO_PROC = 0 };
    ThreadPool (uint8_t numOfProc = AUTO_PROC);

    void Start();
    void Join();

    uint8_t GetNumOfProc() const { return numOfProc; }
    void EnqueueTask (winthread::task *pt);
};


#endif /* _THREADPOOL_HPP_ */
