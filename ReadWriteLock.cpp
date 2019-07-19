#include "ReadWriteLock.h"
#include <assert.h>

RecursiveReadWriteLock::RecursiveReadWriteLock()
    :   m_read(0),
        m_write(0),
        m_lockedBy(0)
{

}

RecursiveReadWriteLock::~RecursiveReadWriteLock()
{

}

void RecursiveReadWriteLock::readLock()
{
    ThreadID threadID = getCurrentThreadID();
    if (threadID != m_lockedBy)
    {
        for (;;)
        {
            while(m_write.load()){}

            ++m_read;
            if (m_write.load())
                --m_read;
            else
                break;
        }
    }
    else
    {
        ++m_read;
    }
}

bool RecursiveReadWriteLock::tryReadLock()
{
    ThreadID threadID = getCurrentThreadID();
    if (threadID != m_lockedBy)
    {
        ++m_read;
        if (m_write.load())
        {
            --m_read;
            return false;
        }
    }
    else
    {
        ++m_read;
    }
    return true;
}

void RecursiveReadWriteLock::readUnlock()
{
    --m_read;
}

void RecursiveReadWriteLock::writeLock()
{
    ThreadID threadID = getCurrentThreadID();
    if (threadID != m_lockedBy)
    {
        int v = 0;
        while(!m_write.compare_exchange_strong(v, 1))
        {
            v = 0;
        }
        while (m_read.load()){}
        m_lockedBy = threadID;
    }
    else
    {
        ++m_write;
    }
}

bool RecursiveReadWriteLock::tryWriteLock()
{
    ThreadID threadID = getCurrentThreadID();
    if (threadID != m_lockedBy)
    {
        int v = 0;
        if(!m_write.compare_exchange_strong(v, 1))
            return false;

        if (m_read.load())
        {
            m_write = 0;
            return false;
        }

        m_lockedBy = threadID;
    }
    else
    {
        ++m_write;
    }

    return true;
}

void RecursiveReadWriteLock::writeUnlock()
{
    assert(m_lockedBy == getCurrentThreadID());
    readLock();
    if(!--m_write)
        m_lockedBy = 0;
    readUnlock();
}

RecursiveReadWriteLock::ThreadID RecursiveReadWriteLock::getCurrentThreadID()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#else
    return syscall(SYS_gettid);
#endif
}


ReadWriteLock::ReadWriteLock()
    :   m_read(0),
        m_write(0)
{

}

ReadWriteLock::~ReadWriteLock()
{

}

void ReadWriteLock::readLock()
{
    for (;;)
    {
        while(m_write.load()){}

        ++m_read;
        if (m_write.load())
            --m_read;
        else
            break;
    }
}

bool ReadWriteLock::tryReadLock()
{
    ++m_read;
    if (m_write.load())
    {
        --m_read;
        return false;
    }

    return true;
}

void ReadWriteLock::readUnlock()
{
    --m_read;
}

void ReadWriteLock::writeLock()
{
    int v = 0;
    while(!m_write.compare_exchange_strong(v, 1))
    {
        v = 0;
    }
    while (m_read.load()){}
}

bool ReadWriteLock::tryWriteLock()
{
    int v = 0;
    if(!m_write.compare_exchange_strong(v, 1))
        return false;

    if (m_read.load())
    {
        m_write = 0;
        return false;
    }

    return true;
}

void ReadWriteLock::writeUnlock()
{
    --m_write;
}
