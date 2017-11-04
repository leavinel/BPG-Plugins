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


/**
 * WorkerThread management pool
 */
class ThreadPool
{
private:
    uint8_t numOfProc;
    bool bStarted;
    winthread::mutex taskMtx;
    winthread::cond_var taskCv;
    std::queue<std::function<void()>> tasks;
    std::unique_ptr<winthread::thread[]> threads;
    void dequeueTask (std::function<void()> &task);
    void threadProc();

public:
    enum { AUTO_PROC = 0 };
    ThreadPool (uint8_t numOfProc = AUTO_PROC);

    void Start();
    void Join();

    uint8_t GetNumOfProc() const { return numOfProc; }
    void EnqueueTask (const std::function<void()> &task);
};


#endif /* _THREADPOOL_HPP_ */
