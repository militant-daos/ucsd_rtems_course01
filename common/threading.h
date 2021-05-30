#pragma once

#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <set>

#include "error_codes.h"

namespace threading
{
using SchedPolicy = int;
using CpuSet = std::set<size_t>;
using CpuIndex = int;

/**
 * @brief Get PID (= thread ID) for the calling thread.
 *
 * @return PID.
 */
pid_t myThreadId()
{
    return getpid();
}

/**
 * @brief Get index of the CPU which the calling thread
 *        being run on.
 *
 * @return CPU index.
 */
CpuIndex myCpu()
{
    return sched_getcpu();
}

/**
 * @brief Get current scheduling policy for the given thread.
 *
 * @param rtPid PID of the caller.
 *
 * @return Scheduling policy code.
 */
SchedPolicy getSchedulerPolicy(pid_t rtPid)
{
    return sched_getscheduler(rtPid);
}

/**
 * @brief Get current scheduling policy for the current thread..
 *
 * @param rtPid PID of the caller.
 *
 * @return Scheduling policy code.
 */
SchedPolicy getCurrThreadSchedulerPolicy()
{
    return getSchedulerPolicy(myThreadId());
}

/**
 * @brief Get an alphanumeric name for the given scheduling policy code.
 *
 * @param rdSchedPolicy Scheduling policy code.
 *
 * @return Name of the policy represented as a string.
 */
const char* getSchedulerPolicyStr(SchedPolicy rdSchedPolicy)
{
    switch (rdSchedPolicy)
    {
        case SCHED_OTHER:
            return "SCHED_OTHER";
        case SCHED_BATCH:
            return "SCHED_BATCH";
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
        default:
            return "SCHED_IDLE";
    }
}

/**
 * @brief Adjust scheduler according to the given params.
 *        Also sets the max priority for the given
 *        scheduling policy.
 *
 * @param[in] rtCpuSet CPU set to be used (cores indices).
 * @param[in] rtNewPolicy New scheduling policy to apply.
 * @param[out] rtAdjustedAttr Thread attributes initialized after scheduler adjusting
 *                            to be passed to future pthread_create calls.
 * @param[in] rbVerbose If true - print debug output.
 *
 * @return Error code.
 */
cmn::ErrCode adjustScheduler(const CpuSet& rtCpuSet, SchedPolicy rtNewPolicy, pthread_attr_t& rtAdjustedAttr, bool rbVerbose = false)
{
    if (rbVerbose)
    {
        CMN_LOG_TRACE("Initial sched policy %s", getSchedulerPolicyStr(getCurrThreadSchedulerPolicy()));
    }

    pthread_attr_init(&rtAdjustedAttr);

    // Do not inherit parent thread's schedulting attributes
    // since we're setting them explicitly.
    RET_ON_ERR(pthread_attr_setinheritsched(&rtAdjustedAttr, PTHREAD_EXPLICIT_SCHED),
            "pthread_attr_setinheritsched call failed with err ");
    RET_ON_ERR(pthread_attr_setschedpolicy(&rtAdjustedAttr, rtNewPolicy),
            "pthread_attr_setschedpolicy call failed with err ");

    // If no CPU indices given - do not alter the currently active CPU set.
    if (!rtCpuSet.empty())
    {
        cpu_set_t tActualCpuSet {};
        CPU_ZERO(&tActualCpuSet);

        // Iterate through the input set and push
        // the CPU indices to the actually used set.
        for (const auto dCpuIndex : rtCpuSet)
        {
            CPU_SET(dCpuIndex, &tActualCpuSet);
        }

        RET_ON_ERR(pthread_attr_setaffinity_np(&rtAdjustedAttr, sizeof(rtAdjustedAttr), &tActualCpuSet),
                "Failed to set affinity with err ");
    }

    sched_param tSchedParam {};
    tSchedParam.sched_priority = sched_get_priority_max(rtNewPolicy);

    RET_ON_ERR(sched_setscheduler(myThreadId(), rtNewPolicy, &tSchedParam),
            "Failed to set scheduling policy, err ");
    RET_ON_ERR(pthread_attr_setschedparam(&rtAdjustedAttr, &tSchedParam),
            "Failed to set sched param, err ");

    if (rbVerbose)
    {
        CMN_LOG_TRACE("Adjusted sched policy %s", getSchedulerPolicyStr(getCurrThreadSchedulerPolicy()));
    }

    return cmn::ErrCode::OK;
}
}
