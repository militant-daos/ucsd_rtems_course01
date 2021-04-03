#include <utility>

#include <errno.h>

// Common header which contains Syslog helpers
// and some other auxiliary stuff.
#include "common.h"
using namespace cmn;

namespace
{
constexpr auto SYSLOG_LABEL = "[COURSE:1][ASSIGNMENT:1]";
}

/**
 * @brief Create and run a worker thread.
 *
 * @param[out] rtEntry Thread entry.
 *
 * @return Error code.
 */
ErrCode spawnWorkerThread(pthread_t& rtEntry)
{
    // Write Hello... from the main thread before spawning a worker one.
    syslog(LOG_DEBUG, "Hello World from Main!");

    // Default attrs, zero-initialized.
    pthread_attr_t tDefaultAttr {};
    pthread_attr_init(&tDefaultAttr);

    const auto dErr = pthread_create(&rtEntry, // pthread handle.
                                     &tDefaultAttr,

                                     // Thread func.
                                     [](void*) -> void*
                                     {
                                         syslog(LOG_DEBUG, "Hello World from Thread!");
                                         pthread_exit(nullptr);

                                         // No need to return - nullptr seems to be returned implicitly.
                                     },

                                     // Thread args object pointer.
                                     nullptr);
    if (dErr != 0)
    {
        std::cerr << "Failed to create the worker thread. Error: %d" << dErr;
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


    // Check the Syslog status code and spawn the worker threds.
    pthread_t tEntry;
    if ((ErrCode::OK != tSyslogErr) || (ErrCode::OK != spawnWorkerThread(tEntry)))
    {
        exit(EXIT_FAILURE);
    }

    // Wait for the worker thread to join.
    pthread_join(tEntry, nullptr);

    std::cout << "TEST COMPLETE" << std::endl;
    exit(EXIT_SUCCESS);
}
