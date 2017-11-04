/**
 * @file
 *
 *
 * @author Leav Wu (leavinel@gmail.com)
 */
#ifndef _WINTHREAD_HPP_
#define _WINTHREAD_HPP_


#include <windows.h>
#include <list>
#include <functional>


/**
 * Subset of windows thread synchronization C++ wrapper
 */
namespace winthread {

/**
 * Wrapper of windows HANDLE
 */
class handle
{
protected:
    HANDLE h;

public:
    handle(): h(NULL) {}
    ~handle();

    void wait (DWORD timeout = INFINITE);
};


class mutex: private handle
{
public:
    mutex();
    void lock() { wait(); }
    void unlock();
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

    event (int opts = 0);
    void wait() { handle::wait(); }
    void signal();
    void reset();
};


class cond_var
{
private:
    mutex waitMtx;
    std::list<event*> waitList;

public:
    cond_var() {}

    /** Mutex must be locked before call */
    void wait (mutex &mtx);
    void notify_one();
    void notify_all();
};



class thread: private handle
{
private:
    std::function<void()> task;
    volatile bool b_joinable;
    unsigned threadId;
    mutex joinMtx;
    event beginEv;

public:
    thread(): b_joinable(false), threadId(0), beginEv(event::OPT_MANUAL_RESET) {}

    virtual ~thread();

    bool joinable();
    void start (std::function<void()> &task);
    void join();

private:
    unsigned procedure();
    static unsigned WINAPI _procedure (void *_thiz);
};


}



#endif /* _WINTHREAD_HPP_ */
