#ifndef DEADLOCKCHECKER_H
#define DEADLOCKCHECKER_H

#include <string>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include <list>
#include <thread>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <processthreadsapi.h>
#else
    #include <sys/syscall.h>
    #include <unistd.h>
#endif

class DeadlockChecker
{
#if defined(_WIN32) || defined(_WIN64)
    typedef DWORD ThreadID;
#else
    typedef __pid_t ThreadID;
#endif

    struct SourceCodePosition
    {
        std::string filename;
        int line;
    };

    struct Lock
    {
        void* p;
        bool isRecursive;
        bool isReadWriteLock;
        SourceCodePosition firstPos;

        std::map<ThreadID, int> countLock;
        std::map<ThreadID, int> countReadLock;
        std::map<ThreadID, int> countWriteLock;
    };


    struct PositionLock
    {
        void* p;
        SourceCodePosition pos;
        int flagLock;
        bool isLockAction;
    };

    struct LockPath
    {
        struct Count
        {
            int c[4];
        };

        std::list<PositionLock> path;
        std::map<void*, Count> count;
    };

public:
    static void init();
    static DeadlockChecker* share();
    static void release();

    void lock();
    void unlock();

    bool checkLock(void* p, const char *filename, int line, std::string& err);
    bool checkTryLock(void* p, const char *filename, int line, std::string& err);
    bool checkRecursiveLock(void* p, const char *filename, int line, std::string& err);
    bool checkRecursiveTryLock(void* p, const char *filename, int line, std::string& err);
    bool checkUnlock(void* p, const char *filename, int line, std::string& err);

    bool checkReadLock(void* p, const char *filename, int line, std::string& err);
    bool checkRecursiveReadLock(void* p, const char *filename, int line, std::string& err);
    bool checkReadUnlock(void* p, const char *filename, int line, std::string& err);

    bool checkWriteLock(void* p, const char *filename, int line, std::string& err);
    bool checkRecursiveWriteLock(void* p, const char *filename, int line, std::string& err);
    bool checkWriteUnlock(void* p, const char *filename, int line, std::string& err);


private:
    inline Lock& getLock(void* p, bool isRecursive, bool isReadWriteLock, const char *filename, int line);
    inline Lock& getLock(void* p);

    ThreadID getCurrentThreadID();
    LockPath& getLockPath(ThreadID threadID);
    inline bool isIntersect(const LockPath& path, void* p, int flagLock, const LockPath& path2);

    std::string stringOfDeadlock(void *p,  const char *filename, int line,
                ThreadID threadID1, const LockPath& path1,
                ThreadID threadID2, const LockPath& path2);
    std::string stringOfDeadlock(ThreadID threadID, const LockPath& path);

    std::string stringOfError(const char *err, const char *filename, int line);

    inline void record(ThreadID threadID, void* p, const char *filename, int line, std::map<ThreadID, int>& counter, LockPath& path, int flagLock);

    bool doCheckLock(void* p, const char *filename, int line, std::string& err, int flagLock, bool isRecursive);
    bool doCheckUnlock(void* p, const char *filename, int line, std::string& err, int flagLock);

    bool doCheckLock(void* p, const char *filename, int line, std::string& err, int flagLock, bool isRecursive, ThreadID currentthreadID);
    bool doCheckUnlock(void* p, const char *filename, int line, std::string& err, int flagLock, ThreadID currentthreadID);

private:
    DeadlockChecker();
    ~DeadlockChecker();

private:
    std::map<void*, Lock> m_locks;
    std::map<ThreadID, LockPath> m_lockPath;

    std::recursive_mutex m_mutex;

    static DeadlockChecker* s_this;
};

#ifdef ENABLE_DEADLOCK_CHECK

#define DEADLOCK_CHECK_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_TRY_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkRecursiveLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_TRY_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkRecursiveLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_UNLOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkUnlock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\


#define DEADLOCK_CHECK_READ_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkReadLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_TRY_READ_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkReadLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_READ_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkRecursiveReadLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_TRY_READ_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkRecursiveReadLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_READ_UNLOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkReadUnlock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\


#define DEADLOCK_CHECK_WRITE_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkWriteLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_TRY_WRITE_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkWriteLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_WRITE_LOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkRecursiveWriteLock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_RECURSIVE_TRY_WRITE_LOCK(__mutex, __func, __err) \
    ({\
        DeadlockChecker::share()->lock();\
        bool ret = (__mutex).__func();\
        if (ret)\
        {\
            bool success = DeadlockChecker::share()->checkRecursiveWriteLock(&(__mutex), __FILE__, __LINE__, __err);\
            assert(success);\
        }\
        DeadlockChecker::share()->unlock();\
        ret;\
    })\

#define DEADLOCK_CHECK_WRITE_UNLOCK(__mutex, __func, __err) \
    ({\
        bool ret = DeadlockChecker::share()->checkWriteUnlock(&(__mutex), __FILE__, __LINE__, __err);\
        if (ret)\
            (__mutex).__func();\
        ret;\
    })\

#else
#define DIRECT_LOCK(__mutex, __func, __err)  \
    ({\
        (__mutex).__func();\
        true;\
    })\

#define DIRECT_TRY_LOCK(__mutex, __func, __err)  \
    ({\
        bool ret = (__mutex).__func();\
        ret;\
    })\

#define DEADLOCK_CHECK_LOCK(__mutex, __func, __err)         DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_TRY_LOCK(__mutex, __func, __err)         DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_LOCK(__mutex, __func, __err)       DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_TRY_LOCK(__mutex, __func, __err)       DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_UNLOCK(__mutex, __func, __err)       DIRECT_LOCK(__mutex, __func, __err)


#define DEADLOCK_CHECK_READ_LOCK(__mutex, __func, __err)        DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_TRY_READ_LOCK(__mutex, __func, __err)        DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_READ_LOCK(__mutex, __func, __err)      DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_TRY_READ_LOCK(__mutex, __func, __err)      DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_READ_UNLOCK(__mutex, __func, __err)      DIRECT_LOCK(__mutex, __func, __err)

#define DEADLOCK_CHECK_WRITE_LOCK(__mutex, __func, __err)       DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_TRY_WRITE_LOCK(__mutex, __func, __err)       DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_WRITE_LOCK(__mutex, __func, __err)         DIRECT_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_RECURSIVE_TRY_WRITE_LOCK(__mutex, __func, __err)         DIRECT_TRY_LOCK(__mutex, __func, __err)
#define DEADLOCK_CHECK_WRITE_UNLOCK(__mutex, __func, __err)         DIRECT_LOCK(__mutex, __func, __err)
#endif
#endif // DEADLOCKCHECKER_H
