/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */

#include <stdlib.h>

#include "threadpool.hpp"


using namespace std;
using namespace winthread;


/**
 * Main procedure of worker thread
 */
void ThreadPool::threadProc()
{
    while (1)
    {
        function<void()> task;

        dequeueTask (task);

        if (!task) // Terminate
            break;

        task();
    }
}


ThreadPool::ThreadPool (uint8_t num): numOfProc(num), bStarted(false)
{
    /* If no number of process specified, get it from system */
    if (numOfProc == AUTO_PROC)
    {
        numOfProc = 1;

        const char *s_num = getenv ("NUMBER_OF_PROCESSORS");
        if (s_num)
            numOfProc = atoi (s_num);
    }
}


void ThreadPool::Start()
{
    if (bStarted)
        return;

    if (numOfProc > 1)
    {
        threads = unique_ptr<winthread::thread[]> (new winthread::thread[numOfProc]);

        for (int i = 0; i < numOfProc; i++)
        {
            function<void()> t = bind (&threadProc, this);
            threads[i].start (t);
        }
    }

    bStarted = true;
}


void ThreadPool::Join()
{
    function<void()> term; // Terminator

    if (!bStarted)
        return;

    if (numOfProc > 1)
    {
        /* Insert terminator task to all threads */
        for (int i = 0; i < numOfProc; i++)
            EnqueueTask (term);

        /* Wait all threads done */
        for (int i = 0; i < numOfProc; i++)
        {
            if (threads[i].joinable())
                threads[i].join();
        }
    }
}


void ThreadPool::EnqueueTask (const function<void()> &task)
{
    if (!bStarted)
        Start();

    if (numOfProc == 1)
    {
        /* Single thread, execute immediately */
        if (task)
            task();
    }
    else // Multi-thread
    {
        lock_guard lck(taskMtx);
        tasks.push (task);
        taskCv.notify_one();
    }
}


/**
 * Wait and dequeue incoming task
 */
void ThreadPool::dequeueTask (function<void()> &task)
{
    lock_guard _l(taskMtx);

    /* Wait if no task available */
    while (tasks.size() == 0)
        taskCv.wait (taskMtx);

    /* Dequeue the front task */
    task = tasks.front();
    tasks.pop();
}
