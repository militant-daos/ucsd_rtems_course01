#include <array>
#include <utility>

#include <errno.h>

// Common header which contains Syslog helpers
// and some other auxiliary stuff.
#include "common.h"
using namespace cmn;

struct ThreadArgs
{
    size_t dThreadIdx;
};

// Thread entry object to keep both a thread handle
// and the corresponding args structure together.
using ThreadEntry = std::pair<ThreadArgs, pthread_t>;

namespace
{
constexpr auto SYSLOG_LABEL = "[COURSE:1][ASSIGNMENT:2]";

// Start value for thread index passed in the args.
constexpr size_t THREADS_START_IDX = 1;
constexpr size_t NUM_THREADS = 128;

// Global thread objects container:
// a pair of thread args struct and pthread handle.
using ThreadsArray = std::array<ThreadEntry, NUM_THREADS>;
ThreadsArray aThreads;
}

/**
 * @brief Create and run worker threads. The number of
 *        threads to spawn is defined by NUM_THREADS.
 *
 * @return Error code.
 */
ErrCode spawnThreads()
{
    // Default attrs, zero-initialized.
    pthread_attr_t tDefaultAttr {};
    pthread_attr_init(&tDefaultAttr);

    size_t dIdx = THREADS_START_IDX;

    // Spawn the threads one by one.
    for (auto& tEntry : aThreads)
    {
        tEntry.first.dThreadIdx = dIdx++;
        const auto dErr = pthread_create(&tEntry.second, // pthread handle.
                                         &tDefaultAttr,

                                         // Thread func.
                                         [](void* pThreadParams) -> void*
                                         {
                                             const auto dIdx = reinterpret_cast<ThreadArgs*>(pThreadParams)->dThreadIdx;
                                             size_t dSum = 0;

                                             // Synthetic workload: sum the numbers from 1 to thread IDX.
                                             for (size_t i = 1; i < (dIdx + 1); ++i)
                                             {
                                                 dSum += i;
                                             }
                                             syslog(LOG_DEBUG, "Thread idx=%lu, sum[1..%lu]=%lu", dIdx, dIdx, dSum);
                                             pthread_exit(nullptr);

                                             // No need to return - nullptr seems to be returned implicitly.
                                         },

                                         // Thread args object pointer.
                                         reinterpret_cast<void*>(&tEntry.first));
        if (dErr != 0)
        {
            std::cerr << "Failed to create thread %lu," << tEntry.first.dThreadIdx << " error: %d" << dErr;
            return ErrCode::PTHREAD_ERR;
        }
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


    // Check the Syslog status code and spawn the worker threds.
    if ((ErrCode::OK != tSyslogErr) || (ErrCode::OK != spawnThreads()))
    {
        exit(EXIT_FAILURE);
    }

    // Wait for completion for all the threads spawned above.
    for (const auto& tEntry : aThreads)
    {
        pthread_join(tEntry.second, nullptr);
    }

    std::cout << "TEST COMPLETE" << std::endl;
    exit(EXIT_SUCCESS);
}
