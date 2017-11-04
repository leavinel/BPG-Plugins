/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#include <process.h>
#include <exception>
#include "winthread.hpp"

using namespace std;
using namespace winthread;

handle::~handle()
{
    CloseHandle (h);
    h = NULL;
}

void handle::wait (DWORD timeout)
{
    if (WaitForSingleObject (h, timeout) != WAIT_OBJECT_0)
        DebugBreak();
}


mutex::mutex()
{
    h = CreateMutexA (NULL, FALSE, NULL);
}


void mutex::unlock()
{
    ReleaseMutex (h);
}


event::event (int opts)
{
    h = CreateEvent (
        NULL,
        (opts & OPT_MANUAL_RESET)  ? TRUE : FALSE,
        (opts & OPT_INIT_SIGNALED) ? TRUE : FALSE,
        NULL
    );
}

void event::signal()
{
    SetEvent (h);
}

void event::reset()
{
    ResetEvent (h);
}


void cond_var::wait (mutex &mtx)
{
    event ev;

    {
        lock_guard _l(waitMtx);
        waitList.push_back (&ev);
        mtx.unlock();
    }
    ev.wait();
    mtx.lock();
}


void cond_var::notify_one()
{
    lock_guard _l(waitMtx);

    if (waitList.size() > 0)
    {
        waitList.front()->signal();
        waitList.pop_front();
    }
}


void cond_var::notify_all()
{
    lock_guard _l(waitMtx);

    while (waitList.size() > 0)
    {
        waitList.front()->signal();
        waitList.pop_front();
    }
}


thread::~thread()
{
    if (b_joinable)
        TerminateThread (h, 0);
}


bool thread::joinable()
{
    beginEv.wait();
    return b_joinable;
}


void thread::start (function<void()> &task)
{
    if (b_joinable)
        //throw runtime_error ("thread already started");
        return;

    this->task = task;
    b_joinable = true;

    h = (HANDLE) _beginthreadex (
        NULL,
        0,
        _procedure,
        reinterpret_cast<void*>(this),
        0,
        &threadId
    );
}


void thread::join()
{
    beginEv.wait();
    wait();
}


unsigned thread::procedure()
{
    beginEv.signal();

    if (task)
        task();

    b_joinable = false;
    return 0;
}


/**
 * Entry point to _beginthreadex()
 */
unsigned WINAPI thread::_procedure (void *_thiz)
{
    try {
        thread *thiz = reinterpret_cast<thread*>(_thiz);
        return thiz->procedure();
    }
    catch (const exception &e) {
        DebugBreak();
        return 0;
    }
}
