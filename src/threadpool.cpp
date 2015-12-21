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


void WorkerThread::run()
{
    mutex &taskMtx = pool.taskMtx;
    cond_var &taskCv = pool.taskCv;
    queue<task*> &tasks = pool.tasks;
    task *pt;

    while (1)
    {
        /* Wait if no task available */
        {
            lock_guard _l(taskMtx);

            while (tasks.size() == 0)
                taskCv.wait (taskMtx);

            /* Dequeue 1 task */
            pt = tasks.front();
            tasks.pop();

            if (!pt) // NULL task means terminate
                break;
        }

        pt->run();
    }
}


void WorkerThread::start()
{
    thread::start();
}


void WorkerThread::join()
{
    if (joinable())
        thread::join();
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
        for (int i = 0; i < numOfProc; i++)
            threads.push_back (make_shared<WorkerThread>(*this));

        for (int i = 0; i < numOfProc; i++)
            threads[i]->start();
    }

    bStarted = true;
}


void ThreadPool::Join()
{
    if (!bStarted)
        return;

    if (numOfProc > 1)
    {
        for (int i = 0; i < numOfProc; i++)
            EnqueueTask (NULL);

        for (int i = 0; i < numOfProc; i++)
            threads[i]->join();
    }
}


void ThreadPool::EnqueueTask (task *pt)
{
    if (!bStarted)
        Start();

    if (numOfProc == 1) // Single-thread
    {
        pt->run();
    }
    else // Multi-thread
    {
        lock_guard lck(taskMtx);
        tasks.push (pt);
        taskCv.notify_one();
    }
}

