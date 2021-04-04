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
    GENERAL_ERR, // A general error, like a Syslog interaction issue.
    LAST // Enum sentinel.
};

// A general-purpose macro to handle standard POSIX calls
// returning errors according to the well-known scheme
// 0 = OK or -ERR_CODE.

#define RET_ON_ERR(f,msg)\
    do\
    {\
        const auto dErr = f;\
        if (dErr != 0)\
        {\
            std::cerr << msg << dErr << std::endl;\
            return cmn::ErrCode::GENERAL_ERR;\
        }\
    }\
    while(0);

}
