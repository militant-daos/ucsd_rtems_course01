#include <array>
#include <utility>

#include <errno.h>
#include <string.h>

// Common header which contains Syslog helpers
// and some other auxiliary stuff.
#include "common.h"

// Scheduler control, CPU info and
// some other threading-related stuff.
#include "threading.h"

using namespace cmn;
using namespace threading;

namespace
{
constexpr auto SYSLOG_LABEL = "[COURSE:1][ASSIGNMENT:3]";

// Start value for thread index passed in the args.
constexpr size_t THREADS_START_IDX = 1;
constexpr size_t NUM_THREADS = 128;

struct ThreadArgs
{
    size_t dThreadIdx;
};

// Thread entry object to keep both a thread handle
// and the corresponding args structure together.
using ThreadEntry = std::pair<ThreadArgs, pthread_t>;
using ThreadsArray = std::array<ThreadEntry, NUM_THREADS>;
}


/**
 * @brief Create and run worker threads. The number of
 *        threads to spawn is defined by NUM_THREADS.
 *
 * @param[in] rtWorkerThreadsAttr Thread attributes structure.
 * @param[in] rpThreadsArray Pointer to ThreadEntry array to hold worker threads entries.
 *
 * @return Error code.
 */
ErrCode spawnThreads(pthread_attr_t& rtWorkerThreadsAttr, ThreadsArray* rpThreadsArray)
{
    size_t dIdx = THREADS_START_IDX;

    // Spawn the threads one by one.
    for (auto& tEntry : *rpThreadsArray)
    {
        tEntry.first.dThreadIdx = dIdx++;
        const auto dErr = pthread_create(&tEntry.second, // pthread handle.
                                         &rtWorkerThreadsAttr,

                                         // Thread func.
                                         [](void* pThreadParams) -> void*
                                         {
                                             const auto dIdx = reinterpret_cast<ThreadArgs*>(pThreadParams)->dThreadIdx;
                                             size_t dSum = 0;

                                             // Synthetic workload: sum the numbers from 1 to thread IDX.
                                             // I do not use any kind of pregression sum formulas intentionally.
                                             for (size_t i = 1; i < (dIdx + 1); ++i)
                                             {
                                                 dSum += i;
                                             }
                                             syslog(LOG_DEBUG, "Thread idx=%zu, sum[1..%zu]=%zu Running on core : %d", dIdx, dIdx, dSum, myCpu());
                                             pthread_exit(nullptr);

                                             // No need to return - nullptr seems to be returned implicitly.
                                         },

                                         // Thread args object pointer.
                                         reinterpret_cast<void*>(&tEntry.first));
        if (dErr != 0)
        {
            std::cerr << "Failed to create thread " << tEntry.first.dThreadIdx << " error: " << dErr << " : " << strerror(dErr) << std::endl;
            return ErrCode::PTHREAD_ERR;
        }
    }

    return ErrCode::OK;
}

// Thread args structure for the starter thread.
struct StarterThreadArgs
{
    pthread_attr_t tThreadAttr;  // Thread attrs to be used for the starter thread
                                 // itself and for the child threads spawned by it.
    ThreadsArray* aThreadsArray; // Pointer to the child threads entries array.
};

/**
 * @brief Spawn the starter thread which cheates all the worker ones.
 *
 * @param[in] rtStarterArgs Starter thread args.
 * @param[out] rtStarterThreadId Starter thread ID.
 *
 * @return Error code.
 */
ErrCode makeStarterThread(StarterThreadArgs& rtStarterArgs, pthread_t& rtStarterThreadId)
{
    const auto dErr = pthread_create(&rtStarterThreadId,
                                     &rtStarterArgs.tThreadAttr,
                                     [](void* rpThreadParams) -> void*
                                     {
                                         std::cout << "The starter thread is running on CPU " << myCpu() << std::endl;

                                         auto pThreadsAttr = static_cast<StarterThreadArgs*>(rpThreadParams);
                                         const auto tErr = spawnThreads(pThreadsAttr->tThreadAttr, pThreadsAttr->aThreadsArray);
                                         if (ErrCode::OK != tErr)
                                         {
                                             std::cerr << "Cannot spawn the worker threads, err " << static_cast<int>(tErr) << std::endl;
                                         }
                                         else
                                         {
                                             // Wait for completion for all the worker threads spawned above.
                                             for (const auto& tEntry : *pThreadsAttr->aThreadsArray)
                                             {
                                                 pthread_join(tEntry.second, nullptr);
                                             }
                                         }

                                         pthread_exit(nullptr);
                                     },

                                     // Thread args object pointer.
                                     reinterpret_cast<void*>(&rtStarterArgs));

    if (dErr != 0)
    {
        std::cerr << "Failed to spawn the worker threads, error: " << dErr << " : " << strerror(dErr) << std::endl;
        return ErrCode::PTHREAD_ERR;
    }

    return ErrCode::OK;
}

int main(int argc, char* argv[])
{
    const auto tSyslogErr = prepareSyslog(SYSLOG_LABEL);
    Finally tSyslogGuard([]()
            {
                // Close Syslog instance upon exit.
                // Should be called at all times since openlog()
                // always succeeds.
                closelog();
            });

    CpuSet tCpuSet;

    // Add just one core idx to the set: 3 (the fourth one
    // starting from zero), just as the example suggests.
    // All the new threads should be executed on this
    // core only.
    tCpuSet.insert(3);

    pthread_attr_t tWorkerThreadsAttr {};

    // Adjust scheduler params including CPU cores set, priority (implicitly the max one is used)
    // and scheduling policy.
    if (ErrCode::OK != adjustScheduler(tCpuSet, SCHED_FIFO, tWorkerThreadsAttr, true /* verbose mode */))
    {
        exit(EXIT_FAILURE);
    }

    ThreadsArray aThreads; // The container for the worker threads entries.
    StarterThreadArgs tStarterThreadArgs;

    tStarterThreadArgs.tThreadAttr = tWorkerThreadsAttr;
    tStarterThreadArgs.aThreadsArray = &aThreads;
    pthread_t tStarterThread;

    // Check the Syslog status code and spawn the worker threds.
    if ((ErrCode::OK != tSyslogErr) ||
            (ErrCode::OK != makeStarterThread(tStarterThreadArgs, tStarterThread)))
    {
        exit(EXIT_FAILURE);
    }

    // Wait for the starter thread and the worker threads to join.
    pthread_join(tStarterThread, nullptr);

    std::cout << "TEST COMPLETE" << std::endl;
    exit(EXIT_SUCCESS);
}
