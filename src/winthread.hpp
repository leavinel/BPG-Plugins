/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _WINTHREAD_HPP_
#define _WINTHREAD_HPP_


#include <windows.h>
#include <process.h>
#include <list>
#include <exception>


namespace winthread {

class handle
{
protected:
    HANDLE h;

public:
    handle(): h(NULL) {}

    ~handle() {
        CloseHandle (h);
        h = NULL;
    }

    void wait (DWORD timeout = INFINITE) {
        if (WaitForSingleObject (h, timeout) != WAIT_OBJECT_0)
            DebugBreak();
    }

    HANDLE& GetRaw() { return h; }
    HANDLE* GetPtr() { return &h; }
};


class mutex: private handle
{
public:
    mutex() {
        h = CreateMutexA (NULL, FALSE, NULL);
    }

    void lock() {
        wait();
    }

    void unlock() {
        ReleaseMutex (h);
    }
};


class lock_guard
{
private:
    mutex &mtx;

public:
    lock_guard (mutex &mtx): mtx(mtx) {
        mtx.lock();
    }

    ~lock_guard() {
        mtx.unlock();
    }
};


class event: private handle
{
public:
    enum {
        OPT_MANUAL_RESET  = 1 << 0,
        OPT_INIT_SIGNALED = 1 << 1,
    };

    event (int opts = 0) {
        h = CreateEvent (
            NULL,
            (opts & OPT_MANUAL_RESET)  ? TRUE : FALSE,
            (opts & OPT_INIT_SIGNALED) ? TRUE : FALSE,
            NULL
        );
    }

    void wait() {
        handle::wait();
    }

    void signal() {
        SetEvent (h);
    }

    void reset() {
        ResetEvent (h);
    }
};


class cond_var
{
private:
    mutex waitMtx;
    std::list<event*> waitList;

public:
    cond_var() {}

    /** Mutex must be locked before call */
    void wait (mutex &mtx) {
        event ev;

        {
            lock_guard _l(waitMtx);
            waitList.push_back (&ev);
            mtx.unlock();
        }
        ev.wait();
        mtx.lock();
    }

    void notify_one() {
        lock_guard _l(waitMtx);

        if (waitList.size() > 0)
        {
            waitList.front()->signal();
            waitList.pop_front();
        }
    }

    void notify_all() {
        lock_guard _l(waitMtx);

        while (waitList.size() > 0)
        {
            waitList.front()->signal();
            waitList.pop_front();
        }
    }
};


struct task {
    virtual void run() = 0;
};


class thread: public task, private handle
{
private:
    task *ptask;
    volatile bool b_joinable;
    unsigned threadId;
    mutex joinMtx;
    event beginEv;

public:
    thread (task *ptask = NULL):
        ptask(ptask), b_joinable(false), threadId(0), beginEv(event::OPT_MANUAL_RESET) {}

    virtual ~thread() {
        if (b_joinable)
            TerminateThread (h, 0);
    }

    bool joinable() {
        beginEv.wait();
        return b_joinable;
    }

    void start() {
        if (b_joinable)
            return;

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

    void join() {
        beginEv.wait();
        wait();
    }

private:
    unsigned procedure() {
        beginEv.signal();

        if (ptask)
            ptask->run();
        else
            run();

        b_joinable = false;
        return reinterpret_cast<unsigned>(this);
    }

    static unsigned WINAPI _procedure (void *_thiz) {
        try {
            thread *thiz = reinterpret_cast<thread*>(_thiz);
            return thiz->procedure();
        }
        catch (const std::exception &e) {
            DebugBreak();
            return 0;
        }
    }
};


}



#endif /* _WINTHREAD_HPP_ */
