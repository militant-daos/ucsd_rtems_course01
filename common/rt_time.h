#pragma once

#include <cassert>

// A funny fact: timespec seems to appear since C11
// however code in the examples
// (at least http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/RT-Clock/)
// is written in C89.

#include <time.h>

#include "common.h"
#include "error_codes.h"

// Units definitions
#define NSEC_PER_SEC  (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)

// Conversion macros
#define NSEC_TO_SEC(x) ((double)(x)/(double)(NSEC_PER_SEC))
// TODO: add the others.

namespace rt_time
{

/**
 * @brief Clock type IDs.
 */
enum ClockTypeId : clockid_t
{
    RealTime = CLOCK_REALTIME,
    Monotonic = CLOCK_MONOTONIC,
    MonotonicRaw = CLOCK_MONOTONIC_RAW,
    RealTimeCoarse = CLOCK_REALTIME_COARSE,
    MonotonicCoarse = CLOCK_MONOTONIC_COARSE
};

/**
 * @brief Return human-readable name for the given clock type ID.
 *
 * @param reClockTypeId Clock type ID.
 *
 * @return String-form type ID.
 */
const char* clockIdToString(ClockTypeId reClockTypeId)
{
    switch (reClockTypeId)
    {
        case ClockTypeId::RealTime:
            return "RealTime";

        case ClockTypeId::Monotonic:
            return "Monotonic";

        case ClockTypeId::MonotonicRaw:
            return "MonotonicRaw";

        case ClockTypeId::RealTimeCoarse:
            return "RealTimeCoarse";

        case ClockTypeId::MonotonicCoarse:
            return "MonotonicCoarse";

        default:
            // Add an assertion maybe?
            return "Unknown Type";
    }
}

/**
 * @brief Compute time points diff in seconds.
 *
 * @param rtStart Start time point.
 * @param rtStop Stop time point.
 *
 * @return Diff in seconds.
 */
double timeDiffInSeconds(const timespec& rtStart, const timespec& rtStop)
{
    // Convert both start and end points to seconds.
    const auto dDfStart = static_cast<double>(rtStart.tv_sec) +
        NSEC_TO_SEC(rtStart.tv_nsec);
    const auto dDfStop = static_cast<double>(rtStop.tv_sec) +
        NSEC_TO_SEC(rtStop.tv_nsec);

    // Double-check to prevent an overflow due to
    // a design-time error.
    assert(dDfStop >= dDfStart);
    return dDfStop - dDfStart;
}

/**
 * @brief Compute time diff as a timespec structure.
 *
 * @param rtStart Start time point.
 * @param rtStop Stop time point.
 * @param rtDiff Diff output structure ref.
 * @param rbIgnoreNegDelta If set to true - ignore dDeltaNsec < 0 case - return OK instead
 *                         of printing an error and failing. For certain clock types this
 *                         case may actually happen and it is Ok.
 *                         In http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/RT-Clock/
 *                         this happens for all clock types except MonotonicRaw but the test
 *                         does not stop because return code from delta_t() call is ignored :-)
 *
 * @return Status code.
 */
cmn::ErrCode timeDiffInTimespec(const timespec& rtStart, const timespec& rtStop, timespec& rtDiff,
        bool rbIgnoreNegDelta = false)
{
    // The original code http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/RT-Clock/
    // uses int type but this is not a good approach since we do narrowing
    // conversion in such a case and if we have a long time period we may get
    // an overflow for tv_nsec. Let's use timespec's original types.

    const long dDeltaSec = rtStop.tv_sec - rtStart.tv_sec;
    long int dDeltaNsec = rtStop.tv_nsec - rtStart.tv_nsec;

    if (dDeltaSec == 0)
    {
        if (dDeltaNsec < 0)
        {
            // The end point occurs earlier than the start.

            if (not rbIgnoreNegDelta)
            {
                CMN_LOG_ERROR("Delta-Ns overflow: %ld (delta seconds == 0)", dDeltaNsec);
                return cmn::ErrCode::OVERFLOW;
            }

            return cmn::ErrCode::OK;
        }

        // Case 1: the time span is less than a second.

        // dDeltaNsec > NSEC_PER_SEC check may be redundant (?) since
        // according to the spec tv_nsec may NOT exceed 1 second
        // in case it is obtained with clock_gettime(); please
        // see https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html ,
        // paragraph 2.8.5.
        if (dDeltaNsec >= 0 && dDeltaNsec < NSEC_PER_SEC)
        {
            rtDiff.tv_sec = 0;
            rtDiff.tv_nsec = dDeltaNsec;
        }
        else // dDeltaNsec > NSEC_PER_SEC - rollover, correct to +1 second.
        {
            // An overflow. Unlikely to happen in an average case - please see above.
            // Assume that we have an overflow not more than 1 second ;-)
            rtDiff.tv_sec = 1;
            rtDiff.tv_nsec = dDeltaNsec - NSEC_PER_SEC;
        }
    }
    else
    {
        // Case 2: more than one second span.
        if (dDeltaNsec >= 0 && dDeltaNsec < NSEC_PER_SEC)
        {
            // The normal case.
            rtDiff.tv_sec = dDeltaSec;
            rtDiff.tv_nsec = dDeltaNsec;
        }
        else
        {
            if (dDeltaNsec > NSEC_PER_SEC) // One second rollover.
            {
                ++rtDiff.tv_sec;
                rtDiff.tv_nsec = dDeltaNsec - NSEC_PER_SEC;
            }
            else
            {
                // May get here during DT error computation -
                // a negative rollover, dDeltaNsec < NSEC_PER_SEC.
                rtDiff.tv_sec = dDeltaSec - 1;
                rtDiff.tv_nsec = dDeltaNsec + NSEC_PER_SEC;
            }
        }
    }

    return cmn::ErrCode::OK;
}

/**
 * @brief Get current time.
 *
 * @param rtClockId Clock type ID.
 * @param rtOutput Time conatiner output ref.
 *
 * @return Status code.
 */
cmn::ErrCode getTime(ClockTypeId rtClockId, timespec& rtOutput)
{
    // Need to use C-style cast since even reinterpret_cast does not
    // work despite using enum class with a proper numeric base type,
    // have no idea why. I feel shame for this.
    if (0 != clock_gettime((clockid_t) rtClockId, &rtOutput))
    {
        return cmn::ErrCode::CLOCK_ERROR;
    }

    return cmn::ErrCode::OK;
}

/**
 * @brief Get clock resolution for the given clock type ID.
 *
 * @param rtClockId Clock type ID to get resolution for.
 * @param rtOutput Resolution output container ref. in timespec format.
 *
 * @return Status code.
 */
cmn::ErrCode getClockResolution(ClockTypeId rtClockId, timespec& rtOutput)
{
    // Need to use C-style cast since even reinterpret_cast does not
    // work despite using enum class with a proper numeric base type,
    // have no idea why. I feel shame for this.
    if (0 != clock_getres((clockid_t) rtClockId, &rtOutput))
    {
        return cmn::ErrCode::CLOCK_ERROR;
    }

    return cmn::ErrCode::OK;
}

}
