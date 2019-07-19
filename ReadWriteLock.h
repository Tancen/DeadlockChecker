#ifndef READWRITELOCK_H
#define READWRITELOCK_H

#include <atomic>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
    #include <processthreadsapi.h>
#else
    #include <sys/syscall.h>
    #include <unistd.h>
#endif

class RecursiveReadWriteLock
{
#if defined(_WIN32) || defined(_WIN64)
    typedef DWORD ThreadID;
#else
    typedef __pid_t ThreadID;
#endif

public:
    RecursiveReadWriteLock();
    ~RecursiveReadWriteLock();

    void readLock();
    bool tryReadLock();
    void readUnlock();
    void writeLock();
    bool tryWriteLock();
    void writeUnlock();

private:
    inline ThreadID getCurrentThreadID();

private:
    std::atomic<int> m_read;
    std::atomic<int> m_write;
    ThreadID m_lockedBy;
};

class ReadWriteLock
{
public:
    ReadWriteLock();
    ~ReadWriteLock();

    void readLock();
    bool tryReadLock();
    void readUnlock();
    void writeLock();
    bool tryWriteLock();
    void writeUnlock();

private:
    std::atomic<int> m_read;
    std::atomic<int> m_write;
};

#endif //READWRITELock_H
