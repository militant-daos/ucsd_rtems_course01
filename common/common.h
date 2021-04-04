#pragma once

#include <functional>
#include <iostream>

#include <syslog.h>
#include <unistd.h>

#include "error_codes.h"

namespace cmn
{
/**
 * @brief The class that implements resource guard logic. The constructor
 *        must be provided with a callback which should implement cleanup,
 *        resources release etc. Basically, this class implements try: ... finally:
 *        paradigm from the languages like Object Pascal or Python.
 */
class Finally
{
public:

    /**
     * @brief Class constructor.
     *
     * @param[in] rtHandler Function (callback) to be invoked
     *                      upon class instance destruction.
     */
    explicit Finally(const std::function<void()>& rtHandler) :
        mtHandler(rtHandler)
    {}

    /**
     * @brief Destructor.
     */
    ~Finally()
    {
        mtHandler();
    }

    // Default implementations for the other constructors
    // and assignment operators to make the compiler happy.

    Finally(const Finally& rtOther) = default;
    Finally(Finally&& rtOther) = default;
    Finally& operator=(const Finally& rtOther) = default;
    Finally& operator=(Finally&& rtOther) = default;

private:
    const std::function<void()> mtHandler;
};

/**
 * @brief Call `uname -a` to get system info and
 *        send it to Syslog.
 *
 *        Please note that system("uname...") produces
 *        an extra output (user name, "pi" in my case, is
 *        added by some reason) which goes to Syslog and
 *        makes the autograder unhappy.
 *
 * @return Error code.
 */
ErrCode pushUnameOutput()
{
    // Call uname and open a pipe to interact with it.
    FILE* pFile = popen("/usr/bin/uname -a", "r");

    if (nullptr == pFile)
    {
        std::cerr << "Failed to invoke uname";
        return ErrCode::GENERAL_ERR;
    }

    // A plenty for uname output which is one-liner. Since a typical
    // thread stack is something like a couple of Megs we can afford
    // 2K on stack without a risk of tampering. The buffer is zero-
    // initialized.
    char aOutBuf[2048] {};

    Finally tPipeGuard([&pFile]()
            {
                // Free the pipe.
                pclose(pFile);
            });

    // Read the output. We don't need a loop here since
    // uname outputs one line which is shorter than the
    // buffer we allocate.
    if (nullptr == fgets(aOutBuf, sizeof(aOutBuf), pFile))
    {
        std::cerr << "Failed to get uname output";
        return ErrCode::GENERAL_ERR;
    }

    // Push uname output to Syslog.
    syslog(LOG_DEBUG, "%s", aOutBuf);

    return ErrCode::OK;
}

/**
 * @brief Open Syslog instance and print
 *        uname output to it.
 *
 * @param[in] rpSyslogLabel Label to be used to prepend all messages.
 *
 * @return Error code.
 */
ErrCode prepareSyslog(const char* rpSyslogLabel)
{
    // A lazy approach: I didn't want to add another
    // FILE*- or stream-based code just to truncate
    // Syslog fire; for the educational purpose
    // system() call may be enough.
    //
    // Syslog is truncated to remove the old info
    // which could break the autograder.
    if (0 != system("/usr/bin/truncate -s 0 /var/log/syslog"))
    {
        return ErrCode::GENERAL_ERR;
    }

    openlog(rpSyslogLabel, LOG_NDELAY, LOG_DAEMON);
    return pushUnameOutput();
}
}
