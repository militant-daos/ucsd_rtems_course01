#pragma once

#include <cassert>
#include <cstdio>
#include <vector>
#include <string>

#include <stdarg.h>

namespace str_utils
{

/**
 * @brief Create a string formatted using given format tamplate and va_list args.
 *
 * @param rpFormat Format string.
 * @param rtVarArgs va_list with the values to be formatted.
 *
 * @return Formatted string.
 */
std::string format_va(const char* rpFormat, va_list& rtVarArgs)
{
    assert(rpFormat != nullptr);

    constexpr size_t MAX_RESIZE_ATTEMPTS = 3;
    int dBufLen = 0;
    size_t dAttempt = 0;;

    std::vector<char> tFmtBuffer(256); // Initial "guess"-like buffer size.

    do
    {
        dBufLen = std::vsnprintf(tFmtBuffer.data(), tFmtBuffer.size(), rpFormat, rtVarArgs);
        if (dBufLen != -1)
        {
            break;
        }

        ++dAttempt;
        assert(dAttempt < MAX_RESIZE_ATTEMPTS);

        // Not enough space allocated. Increase the size twice and try again.
        tFmtBuffer.resize(tFmtBuffer.size() * 2);

    } while(true);

    tFmtBuffer[dBufLen] = '\0';

    return std::string(reinterpret_cast<const char*>(tFmtBuffer.data()));
}

/**
 * @brief Format string using printf()-like approach.
 *
 * @param rpFormat Format string.
 * @param ... Values to be formatted.
 *
 * @return Formatted string.
 */
std::string format(const char* rpFormat, ...)
{
    va_list tArgs;
    va_start(tArgs, rpFormat);
    auto tRet = str_utils::format_va(rpFormat, tArgs);
    va_end(tArgs);
    return tRet;
}
}
