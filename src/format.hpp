/**
 * format.hpp
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <sys/time.h>


namespace format {

template <typename T>
T process_arg(T& value) noexcept {
    return value;
}

template <typename T>
T const* process_arg(std::basic_string<T> const& value) noexcept {
    return value.c_str();
}

/// Format strings
template<typename... Args>
std::string fmt(const std::string& format, Args const&... args) {
    const auto fmt = format.c_str();
    const std::size_t size = std::snprintf(
        nullptr, 0, fmt, process_arg(args)...) + 1;
    std::vector<char> buf;
    buf.resize(size);
    std::snprintf(&buf[0], size, fmt, process_arg(args)...);
    return std::string(buf.begin(), buf.end() - 1);
}

/// Get current time
std::string time() {
    timeval tv;
    gettimeofday(&tv, 0);
    std::tm *now = localtime(&tv.tv_sec);
    return fmt(
        "%d-%02d-%02d %02d:%02d:%02d.%03ld",
        now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour,
        now->tm_min, now->tm_sec, tv.tv_usec/1000);
}

}  // namespace format
