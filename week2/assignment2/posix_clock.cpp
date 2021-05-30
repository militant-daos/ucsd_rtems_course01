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

// Time control, conversion macros etc.
#include "rt_time.h"

using namespace cmn;
using namespace threading;
using namespace rt_time;
using namespace str_utils;

namespace
{
constexpr auto SYSLOG_LABEL = "[COURSE:1][ASSIGNMENT:4]";
constexpr size_t MAX_SLEEP_COUNT = 3;
}

void endDelayTest(ClockTypeId reClockTypeId, const timespec& rtStart, const timespec& rtStop,
        const timespec& rtDiff, const timespec& rtError)
{
    const auto dRealDt = timeDiffInSeconds(rtStart, rtStop);

    CMN_LOG_TRACE("%s clock DT seconds = %ld, msec = %ld, usec = %ld, nsec = %ld, sec = %6.9lf",
            clockIdToString(reClockTypeId), rtDiff.tv_sec, rtDiff.tv_nsec / NSEC_PER_MSEC,
            rtDiff.tv_nsec / NSEC_PER_USEC, rtDiff.tv_nsec, dRealDt);

    CMN_LOG_TRACE("%s clock delay error seconds = %ld, nanoseconds = %ld, ms. = %ld",
            clockIdToString(reClockTypeId), rtError.tv_sec, rtError.tv_nsec, rtError.tv_nsec / NSEC_PER_MSEC);
}

ErrCode delayTest(ClockTypeId reClockTypeId)
{
    const bool bIgnoreNegDeltaErrs = reClockTypeId != ClockTypeId::MonotonicRaw;

    timespec tClockResolution {};
    if (getClockResolution(reClockTypeId, tClockResolution) != ErrCode::OK)
    {
        CMN_LOG_ERROR("Failed to get clock resolution for clock type %d", reClockTypeId);
        return ErrCode::TEST_FAILED;
    }

    CMN_LOG_TRACE("POSIX Clock demo using system RT clock with resolution: %ld secs, %ld microsecs, %ld nanosecs",
            tClockResolution.tv_sec,
            (tClockResolution.tv_nsec / 1000),
            tClockResolution.tv_nsec);

    constexpr size_t dTestIterations = 100;
    constexpr size_t dTestSleepSeconds = 0;
    constexpr size_t dTestSleepNanoseconds = NSEC_PER_MSEC * 10;

    timespec tRtcStartTime {};
    timespec tRtcStopTime {};
    timespec tRtcDiff {};
    timespec tDelayError {};

    timespec tSleepTime {};
    timespec tSleepRequested {};
    timespec tRemainingTime {};

    for (size_t dIdx = 0; dIdx < dTestIterations; ++dIdx)
    {
        CMN_LOG_TRACE("Test %u", dIdx);

        tSleepTime.tv_sec = dTestSleepSeconds;
        tSleepTime.tv_nsec = dTestSleepNanoseconds;

        tSleepRequested.tv_sec = tSleepTime.tv_sec;
        tSleepRequested.tv_nsec = tSleepTime.tv_nsec;

        if (getTime(reClockTypeId, tRtcStartTime) != ErrCode::OK)
        {
            CMN_LOG_ERROR("Failed to get RTC start time for iteration %u", dIdx);
            return ErrCode::TEST_FAILED;
        }

        size_t dSleepCount = 0;

        do
        {
            const auto dRc = nanosleep(&tSleepTime, &tRemainingTime);
            if (dRc == 0)
            {
                break;
            }
            else if (dRc != EINTR)
            {
                CMN_LOG_ERROR("nanosleep() call failed with err code %d", dRc);
                return ErrCode::TEST_FAILED;
            }

            // If we got here it means that EINTR was returned from nanosleep()
            // call and our thread was woken up by the scheduler. This is a normal
            // behavior for nanosleep so we need to check how much remaining time
            // left. Clock resolution should matter: with better resolution we
            // should have smaller sleep error diff between the requested time
            // and the actual one.

            tSleepTime.tv_sec = tRemainingTime.tv_sec;
            tSleepTime.tv_nsec = tRemainingTime.tv_nsec;

            ++dSleepCount;
        }
        while (((tRemainingTime.tv_sec > 0) || (tRemainingTime.tv_nsec > 0))
                && (dSleepCount < MAX_SLEEP_COUNT));

        if (getTime(reClockTypeId, tRtcStopTime) != ErrCode::OK)
        {
            CMN_LOG_ERROR("Failed to get RTC stop time for iteration %u", dIdx);
            return ErrCode::TEST_FAILED;
        }

        auto tErr = timeDiffInTimespec(tRtcStartTime, tRtcStopTime, tRtcDiff, bIgnoreNegDeltaErrs);
        if (tErr != ErrCode::OK && not bIgnoreNegDeltaErrs)
        {
            CMN_LOG_ERROR("Failed to compute start-stop diff, err %d", tErr);
            return tErr;
        }

        tErr = timeDiffInTimespec(tSleepRequested, tRtcDiff, tDelayError, bIgnoreNegDeltaErrs);
        if (tErr != ErrCode::OK && not bIgnoreNegDeltaErrs)
        {
            CMN_LOG_ERROR("Failed to compute sleep error diff, err %d", tErr);
            return tErr;
        }

        endDelayTest(reClockTypeId, tRtcStartTime, tRtcStopTime, tRtcDiff, tDelayError);

        // It would be also nice to know how much iterations it took to sleep for the required
        // time span; it should depend on clock resolution and scheduling policy I guess.
        CMN_LOG_TRACE("Sleep count: %u", dSleepCount);
    }

    return ErrCode::OK;
}

ErrCode makeTestThread(pthread_attr_t& rtThreadAttr, pthread_t& rtThreadId)
{
    const auto dErr = pthread_create(&rtThreadId,
                                     &rtThreadAttr,
                                     [](void*) -> void*
                                     {
                                         // Most notable difference in results is between
                                         // ClockTypeId::MonotonicRaw and ClockTypeId::MonotonicCoarse,
                                         // the latter has worse resolution and sleep DT error may reach
                                         // 2 milliseconds.
                                         const auto tRetCode = delayTest(ClockTypeId::MonotonicRaw);
                                         if (tRetCode != ErrCode::OK)
                                         {
                                             CMN_LOG_ERROR("Test failed with code %d", tRetCode);
                                         }
                                         pthread_exit(nullptr);
                                     },

                                     // Thread args object pointer.
                                     nullptr);

    if (dErr != 0)
    {
        CMN_LOG_ERROR("Failed to spawn the test thread: %d (%s)", dErr, strerror(dErr));
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

    // Create an empty CPU set to use the default params
    // in adjustScheduler() and use any of available CPUs.
    CpuSet tCpuSet {};
    pthread_attr_t tWorkerThreadsAttr {};

    // Adjust scheduler params including CPU cores set, priority (implicitly the max one is used)
    // and scheduling policy.
    // There is a possibility to play with different scheduling policies to observe different
    // numbers for DT errors and sleep iterations.
    SchedPolicy tSchedPolicy = SCHED_FIFO;
    if (ErrCode::OK != adjustScheduler(tCpuSet, tSchedPolicy, tWorkerThreadsAttr, true /* verbose mode */))
    {
        exit(EXIT_FAILURE);
    }

    pthread_t tStarterThread;

    // Check the Syslog status code and spawn the worker threds.
    if ((ErrCode::OK != tSyslogErr) ||
            (ErrCode::OK != makeTestThread(tWorkerThreadsAttr, tStarterThread)))
    {
        exit(EXIT_FAILURE);
    }

    // Wait for the starter thread and the worker threads to join.
    pthread_join(tStarterThread, nullptr);

    CMN_LOG_TRACE("TEST COMPLETE");
    exit(EXIT_SUCCESS);
}
