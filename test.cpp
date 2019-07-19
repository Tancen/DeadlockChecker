#include <QCoreApplication>
#include <mutex>
#include "src/DeadlockChecker.h"
#include "ReadWriteLock.h"
#include <functional>

#define TEST(_expression, _expect, _err) \
{\
    bool _ret = (_expression);\
    if (!_ret)\
        printf("%s\n", err.c_str());\
\
    if (_ret != _expect)\
    {\
        printf("failed.\n");\
        return false;\
    }\
}

bool test1()
{


    std::string err;
    std::mutex m1;

    TEST(DEADLOCK_CHECK_LOCK(m1, lock, err), true, err);
    TEST(DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);
    TEST(DEADLOCK_CHECK_UNLOCK(m1, unlock, err), false, err);


    return true;
}

bool test2()
{


    std::string err;
    std::mutex m1, m2;

    TEST (DEADLOCK_CHECK_TRY_LOCK(m1, try_lock, err), true, err);
    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);
    TEST (DEADLOCK_CHECK_LOCK(m1, lock, err), true, err);
    TEST (DEADLOCK_CHECK_TRY_LOCK(m1, try_lock, err), false, err);

    TEST (DEADLOCK_CHECK_LOCK(m2, lock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m2, unlock, err), true, err);


    return true;
}

bool test3()
{

    std::string err;
    std::recursive_mutex m1;

    TEST (DEADLOCK_CHECK_RECURSIVE_LOCK(m1, lock, err), true, err);


    TEST (DEADLOCK_CHECK_RECURSIVE_LOCK(m1, lock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), false, err);

    return true;
}

bool test4()
{

    std::string err;
    std::recursive_mutex m1;
    std::mutex m2;

    TEST (DEADLOCK_CHECK_RECURSIVE_LOCK(m1, lock, err), true, err);


    TEST (DEADLOCK_CHECK_LOCK(m2, lock, err), true, err);


    TEST (DEADLOCK_CHECK_RECURSIVE_LOCK(m1, lock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_LOCK(m2, lock, err), false, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m2, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), true, err);


    TEST (DEADLOCK_CHECK_UNLOCK(m1, unlock, err), false, err);

    return true;
}

bool test5()
{

    ReadWriteLock m1;
    std::string err;

    TEST (DEADLOCK_CHECK_READ_LOCK(m1, readLock, err), true, err);


    TEST (DEADLOCK_CHECK_TRY_READ_LOCK(m1, tryReadLock, err), true, err);


    TEST (DEADLOCK_CHECK_WRITE_LOCK(m1, writeLock, err), false, err);


    TEST (DEADLOCK_CHECK_WRITE_UNLOCK(m1, writeUnlock, err), false, err);


    TEST (DEADLOCK_CHECK_TRY_WRITE_LOCK(m1, tryWriteLock, err), false, err);


    TEST (DEADLOCK_CHECK_READ_UNLOCK(m1, readUnlock, err), true, err);


    TEST (DEADLOCK_CHECK_READ_UNLOCK(m1, readUnlock, err), true, err);


    return true;
}

bool test6()
{
    RecursiveReadWriteLock m1;
    std::string err;

    TEST (DEADLOCK_CHECK_RECURSIVE_READ_LOCK(m1, readLock, err), true, err);


    TEST (DEADLOCK_CHECK_RECURSIVE_WRITE_LOCK(m1, writeLock, err), false, err);


    TEST (DEADLOCK_CHECK_RECURSIVE_TRY_READ_LOCK(m1, tryReadLock, err), true, err);


    TEST (DEADLOCK_CHECK_RECURSIVE_TRY_WRITE_LOCK(m1, tryWriteLock, err), false, err);


    TEST (DEADLOCK_CHECK_WRITE_UNLOCK(m1, writeUnlock, err), false, err);


    TEST (DEADLOCK_CHECK_READ_UNLOCK(m1, readUnlock, err), true, err);


    TEST (DEADLOCK_CHECK_READ_UNLOCK(m1, readUnlock, err), true, err);

    return true;
}

#define TEST2(_expression, _err, _result) \
    if (!(_expression))\
    {\
        printf("failed. %s\n", err.c_str());\
        _result = false;\
        return ;\
    }

bool test7()
{


    RecursiveReadWriteLock rrw;
    ReadWriteLock rw;
    std::recursive_mutex rl;
    std::mutex l;
    auto func1 = [&](bool& result)
    {
        std::string err;
        for (int i = 0; i < 10000; ++i)
        {
            TEST2(DEADLOCK_CHECK_READ_LOCK(rw, readLock,  err), err, result);

            TEST2(DEADLOCK_CHECK_LOCK(l, lock,  err), err, result);
            TEST2(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);

            if (DEADLOCK_CHECK_RECURSIVE_TRY_LOCK(rl, try_lock,  err))
            {
                TEST2(DEADLOCK_CHECK_LOCK(l, lock,  err), err, result);



                TEST2(DEADLOCK_CHECK_RECURSIVE_WRITE_LOCK(rrw, writeLock,  err), err, result);


                TEST2(DEADLOCK_CHECK_RECURSIVE_READ_LOCK(rrw, readLock,  err), err, result);

                TEST2(DEADLOCK_CHECK_READ_UNLOCK(rrw, readUnlock,  err), err, result);


                TEST2(DEADLOCK_CHECK_WRITE_UNLOCK(rrw, writeUnlock,  err), err, result);

                TEST2(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);

                TEST2(DEADLOCK_CHECK_UNLOCK(rl, unlock,  err), err, result);
            }

            TEST2(DEADLOCK_CHECK_READ_UNLOCK(rw, readUnlock,  err), err, result);

        }

        result = true;
    };

    auto func2 = [&](bool& result)
    {
        std::string err;
        for (int i = 0; i < 10000; ++i)
        {
            TEST2(DEADLOCK_CHECK_WRITE_LOCK(rw, writeLock,  err), err, result);
            if(DEADLOCK_CHECK_RECURSIVE_TRY_LOCK(rl, try_lock,  err))
            {
                TEST2(DEADLOCK_CHECK_LOCK(l, lock,  err), err, result);
                TEST2(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);
                TEST2(DEADLOCK_CHECK_UNLOCK(rl, unlock,  err), err, result);
            }

            TEST2(DEADLOCK_CHECK_LOCK(l, lock,  err), err, result);

            if(DEADLOCK_CHECK_RECURSIVE_TRY_WRITE_LOCK(rrw, tryWriteLock,  err))
            {

                TEST2(DEADLOCK_CHECK_RECURSIVE_READ_LOCK(rrw, readLock,  err), err, result);

                TEST2(DEADLOCK_CHECK_READ_UNLOCK(rrw, readUnlock,  err), err, result);


                TEST2(DEADLOCK_CHECK_WRITE_UNLOCK(rrw, writeUnlock,  err), err, result);
            }
            TEST2(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);

            TEST2(DEADLOCK_CHECK_WRITE_UNLOCK(rw, writeUnlock,  err), err, result);

            if (DEADLOCK_CHECK_TRY_LOCK(l, try_lock,  err))
            {
                TEST2(DEADLOCK_CHECK_RECURSIVE_WRITE_LOCK(rrw, writeLock,  err), err, result);
                TEST2(DEADLOCK_CHECK_RECURSIVE_READ_LOCK(rrw, readLock,  err), err, result);
                TEST2(DEADLOCK_CHECK_READ_UNLOCK(rrw, readUnlock,  err), err, result);
                TEST2(DEADLOCK_CHECK_WRITE_UNLOCK(rrw, writeUnlock,  err), err, result);

                TEST2(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);
            }
        }

        result = true;
    };

    const int THREAD_NUM = 8;
    std::thread threads[THREAD_NUM];
    bool results[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i)
        threads[i] = (rand() & 1) ? std::thread(func1, std::ref(results[i]))
                                  : std::thread(func2, std::ref(results[i]));

    for (int i = 0; i < THREAD_NUM; ++i)
        threads[i].join();

    for (int i = 0; i < THREAD_NUM; ++i)
        if (!results[i])
            return false;

    return true;
}

#define TEST3(_expression, _err, _result) \
    if (!(_expression))\
    {\
        printf("failed. %s\n", err.c_str());\
        _result = false;\
        break ;\
    }

bool test8()
{
    std::recursive_mutex l, l2;
    auto func1 = [&](bool& result)
    {
        std::string err;
        int c[2] = {0};

        result = true;
        for (int i = 0; i < 10000; ++i)
        {
            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l, lock,  err), err, result);
            c[0]++;

            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l2, lock,  err), err, result);
            c[1]++;

            TEST3(DEADLOCK_CHECK_UNLOCK(l2, unlock,  err), err, result);
            c[1]--;
            TEST3(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);
            c[0]--;
        }

        if (!result)
        {
            if (c[0])
                l.unlock();

            if (c[1])
                l2.unlock();
        }
    };

    auto func2 = [&](bool& result)
    {
        std::string err;
        int c[2] = {0};

        result = true;
        for (int i = 0; i < 10000; ++i)
        {
            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l, lock,  err), err, result);
            c[0]++;

            TEST3(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);
            c[0]--;

            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l2, lock,  err), err, result);
            c[1]++;

            TEST3(DEADLOCK_CHECK_UNLOCK(l2, unlock,  err), err, result);
            c[1]--;

            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l2, lock,  err), err, result);
            c[1]++;

            TEST3(DEADLOCK_CHECK_RECURSIVE_LOCK(l, lock,  err), err, result);
            c[0]++;

            TEST3(DEADLOCK_CHECK_UNLOCK(l, unlock,  err), err, result);
            c[0]--;

            TEST3(DEADLOCK_CHECK_UNLOCK(l2, unlock,  err), err, result);
            c[1]--;
        }

        if (!result)
        {
            if (c[0])
                l.unlock();

            if (c[1])
                l2.unlock();
        }
    };

    const int THREAD_NUM = 4;
    std::thread threads[THREAD_NUM];
    bool results[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i)
        threads[i] = (rand() & 1) ? std::thread(func1, std::ref(results[i]))
                                  : std::thread(func2, std::ref(results[i]));

    for (int i = 0; i < THREAD_NUM; ++i)
        threads[i].join();

    for (int i = 0; i < THREAD_NUM; ++i)
        if (!results[i])
            return true;

    return false;
}

#define FLAG_DEFAULT   1
#define FLAG_READ   2
#define FLAG_WRITE  4

int main(int argc, char *argv[])
{
    DeadlockChecker::init();

    bool allSuccess = true;
    const int N = 8;
    std::function<bool()> testFuncs[N] = {test1, test2, test3, test4, test5, test6, test7, test8};
    for (int i = 0; i < N; ++i)
    {
        int index = i + 1;
        printf("test %d :\n", index);
        if (testFuncs[i]())
        {
            printf("test %d success\n\n", index);
        }
        else
        {
            allSuccess = false;
            printf("test %d failure\n\n", index);
        }
    }

    if (!allSuccess)
        printf("One or more tests failed.(test 8 is not necessarily successful)\n\n");
    else
        printf("All tests are passed\n\n");

    DeadlockChecker::release();

    return 0;
}
