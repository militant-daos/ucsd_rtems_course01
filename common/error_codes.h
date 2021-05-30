#pragma once

namespace cmn
{
enum class ErrCode : int
{
    OK = 0,
    PTHREAD_ERR = -1, // Pthread creation error.
    NOT_READY = -2,
    NOT_SUPPORTED = -3,
    NOT_ENABLED = -4,
    NOT_IMPLEMENTED = -5,
    ALREADY_ENABLED = -6,
    OVERFLOW = -7,
    SCHED_FAILURE = -8,
    INVALID_ARGS = -9,
    CLOCK_ERROR = -10,
    TEST_FAILED = -11, // A general error code, just before a test is aborted.
    GENERAL_ERR, // A general error, like a Syslog interaction issue.

    LAST // Enum sentinel.
};
}
