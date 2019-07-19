#include "DeadlockChecker.h"
#include <vector>

#define ERR_CHECK_FUNC_NOT_MATCHING "check func not matching"
#define ERR_UNLOCK_AN_INVALID_LOCK "unlock an invalid lock"

#define FLAG_DEFAULT   1
#define FLAG_READ   2
#define FLAG_WRITE  4

#define INDEX_COUNT_ALL    0
#define INDEX_COUNT_DEFAULT    1
#define INDEX_COUNT_READ    2
#define INDEX_COUNT_WRITE   3

#define MAX_PATH_LEN    50

DeadlockChecker* DeadlockChecker::s_this = NULL;

void DeadlockChecker::init()
{
    if (!s_this)
        s_this = new DeadlockChecker();
}

DeadlockChecker *DeadlockChecker::share()
{
    return s_this;
}

void DeadlockChecker::release()
{
    delete s_this;
    s_this = NULL;
}

void DeadlockChecker::lock()
{
    m_mutex.lock();
}

void DeadlockChecker::unlock()
{
    m_mutex.unlock();
}

bool DeadlockChecker::checkLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_DEFAULT, false);
}

bool DeadlockChecker::checkRecursiveLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_DEFAULT, true);
}

bool DeadlockChecker::checkUnlock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckUnlock(p, filename, line, err, FLAG_DEFAULT);
}

bool DeadlockChecker::checkReadLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_READ, false);
}

bool DeadlockChecker::checkRecursiveReadLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_READ, true);
}

bool DeadlockChecker::checkReadUnlock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckUnlock(p, filename, line, err, FLAG_READ);
}

bool DeadlockChecker::checkWriteLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_WRITE, false);
}

bool DeadlockChecker::checkRecursiveWriteLock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckLock(p, filename, line, err, FLAG_WRITE, true);
}

bool DeadlockChecker::checkWriteUnlock(void *p, const char *filename, int line, std::string &err)
{
    return doCheckUnlock(p, filename, line, err, FLAG_WRITE);
}

DeadlockChecker::Lock &DeadlockChecker::getLock(void* p, bool isRecursive, bool isReadWriteLock, const char *filename, int line)
{
    return m_locks.insert(std::make_pair(p, Lock{p, isRecursive, isReadWriteLock, SourceCodePosition{filename, line},
        std::map<ThreadID, int>(), std::map<ThreadID, int>(), std::map<ThreadID, int>()})).first->second;
}

DeadlockChecker::Lock &DeadlockChecker::getLock(void *p)
{
    auto it = m_locks.find(p);
    assert(it != m_locks.end());

    return it->second;
}

DeadlockChecker::ThreadID DeadlockChecker::getCurrentThreadID()
{
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#else
    return syscall(SYS_gettid);
#endif
}

DeadlockChecker::LockPath &DeadlockChecker::getLockPath(DeadlockChecker::ThreadID currentthreadID)
{
    return m_lockPath.insert(std::make_pair(currentthreadID, LockPath())).first->second;
}

bool DeadlockChecker::isIntersect(const LockPath& path, void* p, int flagLock, const LockPath &path2)
{
    if (path2.count.size() < 2 || path.count.empty())
        return false;

    if (path2.path.back().p == p)
        return false;

    auto it1 = path2.count.find(p);
    if (it1 == path2.count.end())
        return false;

    auto it2 = path.count.find(path2.path.back().p);
    if (it2 == path.count.end())
        return false;

    if (flagLock == FLAG_READ && !it2->second.c[INDEX_COUNT_WRITE]
            && !it1->second.c[INDEX_COUNT_WRITE] && path2.path.back().flagLock != FLAG_WRITE)
        return false;

    return true;
}

std::string DeadlockChecker::stringOfDeadlock(void *p, const char *filename, int line,
    DeadlockChecker::ThreadID threadID1, const DeadlockChecker::LockPath &path1,
    DeadlockChecker::ThreadID threadID2, const DeadlockChecker::LockPath &path2)
{
    char buf[256] = {0};

    sprintf(buf, "conflict from thread %x lock: %p", getCurrentThreadID(), p);

    std::string ret = buf;
    ret.append(" (").append(filename).append(":").append(std::to_string(line)).append(") \n");
    ret.append(stringOfDeadlock(threadID1, path1));
    ret.append(stringOfDeadlock(threadID2, path2));
    ret.append("\n");

    ret.append("lock list:\n");
    for (auto it : m_locks)
    {
        Lock& lock = it.second;
        sprintf(buf, "  %-8p", lock.p);
        ret.append(buf);
        if (lock.isRecursive)
            ret.append(" recursive");
        if (lock.isReadWriteLock)
            ret.append(" read-write lock ");
        else
            ret.append(" default ");

        ret.append(" (").append(lock.firstPos.filename).append(":").append(std::to_string(lock.firstPos.line)).append(")\n");

    }

    return ret;
}

std::string DeadlockChecker::stringOfDeadlock(DeadlockChecker::ThreadID currentthreadID, const DeadlockChecker::LockPath &path)
{
    char buf[256] = {0};
    const char* hintIsLockAction[] = { "unlock", "lock"};
    const char* hintFlagLock[FLAG_DEFAULT | FLAG_READ | FLAG_WRITE];
    hintFlagLock[FLAG_DEFAULT] = "";
    hintFlagLock[FLAG_READ] = "read";
    hintFlagLock[FLAG_WRITE] = "write";

    std::string ret = "Thread ";
    sprintf(buf, "%x", currentthreadID);
    ret.append(buf).append(" :\n");
    for (auto it = path.path.rbegin(); it != path.path.rend(); ++it)
    {
        const PositionLock& pos = *it;
        sprintf(buf, "  %8s %-8s %p", hintFlagLock[pos.flagLock], hintIsLockAction[pos.isLockAction], pos.p);
        ret.append(buf).append("  ");
        ret.append(pos.pos.filename).append(":").append(std::to_string(pos.pos.line));
        ret.append("\n");
    }

    ret.append("handling locks: \n");
    if (path.count.empty())
    {
        ret.append("  empty\n");
    }
    else
    {
        for (auto it : path.count)
        {
            sprintf(buf, "  %p default:%d, read:%d, write:%d\n", it.first,
                    it.second.c[INDEX_COUNT_DEFAULT], it.second.c[INDEX_COUNT_READ],
                    it.second.c[INDEX_COUNT_WRITE]);
            ret.append(buf);
        }
    }

    return ret;
}

std::string DeadlockChecker::stringOfError(const char *err, const char *filename, int line)
{
    std::string ret;
    ret.append(err).append(" (").append(filename).append(":").append(std::to_string(line)).append(")");

    return ret;
}

void DeadlockChecker::record(ThreadID threadID, void *p, const char *filename,
    int line, std::map<ThreadID, int> &counter, DeadlockChecker::LockPath &path, int flagLock)
{
    path.path.push_back(PositionLock{p, SourceCodePosition{filename, line}, flagLock, true});
    if (path.path.size() > MAX_PATH_LEN)
        path.path.pop_front();

    LockPath::Count& c = path.count.insert(std::make_pair(p, LockPath::Count{0})).first->second;
    switch (flagLock)
    {
    case FLAG_DEFAULT:
        c.c[INDEX_COUNT_DEFAULT]++;
        break;
    case FLAG_READ:
        c.c[INDEX_COUNT_READ]++;
        break;
    case FLAG_WRITE:
        c.c[INDEX_COUNT_WRITE]++;
        break;

    default:
        assert(0);
    }
    c.c[INDEX_COUNT_ALL]++;

    counter.insert(std::make_pair(threadID, 0)).first->second++;
}

bool DeadlockChecker::doCheckLock(void *p, const char *filename, int line, std::string &err, int flagLock, bool isRecursive)
{
    ThreadID currentthreadID = getCurrentThreadID();
    return doCheckLock(p, filename, line, err, flagLock, isRecursive, currentthreadID);
}

bool DeadlockChecker::doCheckUnlock(void *p, const char *filename, int line, std::string &err, int flagLock)
{
    ThreadID currentthreadID = getCurrentThreadID();
    return doCheckUnlock(p, filename, line, err, flagLock, currentthreadID);
}

bool DeadlockChecker::doCheckLock(void *p, const char *filename, int line, std::string &err, int flagLock,
    bool isRecursive, DeadlockChecker::ThreadID currentthreadID)
{
    std::lock_guard<std::recursive_mutex> lockGuard(m_mutex);

    bool isReadWriteLock = flagLock != FLAG_DEFAULT;
    Lock& lock = getLock(p, isRecursive, isReadWriteLock, filename, line);
    if (lock.isReadWriteLock != isReadWriteLock || lock.isRecursive != isRecursive)
    {
        err = stringOfError(ERR_CHECK_FUNC_NOT_MATCHING, filename, line);
        return false;
    }
    LockPath& currentLockPath = getLockPath(currentthreadID);

    std::map<ThreadID, int> *dstCounter = NULL;
    std::vector<std::map<ThreadID, int>*> vec(2);
    vec.resize(0);
    switch(flagLock)
    {
    case FLAG_DEFAULT:
        vec.push_back(&lock.countLock);
        dstCounter = &lock.countLock;
        break;
    case FLAG_READ:
        vec.push_back(&lock.countWriteLock);
        vec.push_back(&lock.countReadLock);
        dstCounter = &lock.countReadLock;
        break;
    case FLAG_WRITE:
        vec.push_back(&lock.countWriteLock);
        vec.push_back(&lock.countReadLock);
        dstCounter = &lock.countWriteLock;
        break;
    default:
        assert(0);
    }

    if (!currentLockPath.count.empty())
    {
        for (std::map<ThreadID, int>* counter : vec)
        {
            for (auto itThread : *counter)
            {
                if (itThread.first == currentthreadID)
                {
                    std::map<void*, LockPath::Count>::const_iterator it = currentLockPath.count.find(p);
                    if (it != currentLockPath.count.end())
                    {
                        if (flagLock == FLAG_READ)
                            continue;

                        if (isRecursive)
                        {
                            if (flagLock == FLAG_WRITE && !it->second.c[INDEX_COUNT_WRITE])
                            {
                                err = stringOfDeadlock(p, filename, line, currentthreadID, currentLockPath, currentthreadID, currentLockPath);
                                return false;
                            }
                        }
                        else
                        {
                            err = stringOfDeadlock(p, filename, line, currentthreadID, currentLockPath, currentthreadID, currentLockPath);
                            return false;
                        }
                    }
                }
                else
                {
                    LockPath& tmpLockPath = getLockPath(itThread.first);
                    if (isIntersect(currentLockPath, p, flagLock, tmpLockPath))
                    {
                        err = stringOfDeadlock(p, filename, line, currentthreadID, currentLockPath, itThread.first, tmpLockPath);
                        return false;

                    }
                }
            }
        }
    }
    record(currentthreadID, p, filename, line, *dstCounter, currentLockPath, flagLock);

    return true;
}

bool DeadlockChecker::doCheckUnlock(void *p, const char *filename, int line, std::string &err, int flagLock, DeadlockChecker::ThreadID currentthreadID)
{
    std::lock_guard<std::recursive_mutex> lockGuard(m_mutex);

    LockPath& currentLockPath = getLockPath(currentthreadID);
    {
        auto itCount = currentLockPath.count.find(p);
        if (itCount == currentLockPath.count.end())
        {
            err = stringOfError(ERR_UNLOCK_AN_INVALID_LOCK, filename, line);
            return false;
        }

        LockPath::Count& count = itCount->second;
        int* v = NULL;
        switch(flagLock)
        {
        case FLAG_DEFAULT:
            v = &count.c[INDEX_COUNT_DEFAULT];
            break;
        case FLAG_READ:
            v = &count.c[INDEX_COUNT_READ];
            break;
        case FLAG_WRITE:
            v = &count.c[INDEX_COUNT_WRITE];
            break;
        default:
            assert(0);
        }

        if (!(*v))
        {
            err = stringOfError(ERR_UNLOCK_AN_INVALID_LOCK, filename, line);
            return false;
        }

        (*v)--;
        --(count.c[INDEX_COUNT_ALL]);
        assert (count.c[INDEX_COUNT_ALL] >= 0);
        if (!count.c[INDEX_COUNT_ALL])
            currentLockPath.count.erase(itCount);

        currentLockPath.path.push_back(PositionLock{p, SourceCodePosition{ filename, line}, flagLock, false });
        if (currentLockPath.path.size() > MAX_PATH_LEN)
            currentLockPath.path.pop_front();
    }

    {
        Lock& lock = getLock(p);
        std::map<ThreadID, int> *counter = NULL;
        switch(flagLock)
        {
        case FLAG_DEFAULT:
            counter = &lock.countLock;
            break;
        case FLAG_READ:
            counter = &lock.countReadLock;
            break;
        case FLAG_WRITE:
            counter = &lock.countWriteLock;
            break;
        default:
            assert(0);
        }

        auto itCount = counter->find(currentthreadID);
        assert(itCount != counter->end());

        itCount->second--;
        assert(itCount->second >= 0);

        if (!itCount->second)
        {
            counter->erase(itCount);
            if (flagLock == FLAG_DEFAULT)
            {
                if (lock.countLock.empty())
                    m_locks.erase(p);
            }
            else
            {
                if (lock.countReadLock.empty() && lock.countWriteLock.empty())
                    m_locks.erase(p);
            }
        }
    }

    return true;
}

DeadlockChecker::DeadlockChecker()
{

}

DeadlockChecker::~DeadlockChecker()
{

}

