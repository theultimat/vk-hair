#ifndef VHS_TRACE_HPP
#define VHS_TRACE_HPP

#include <chrono>
#include <iostream>
#include <ratio>
#include <utility>

#include <fmt/format.h>


#define VHS_TRACE_DECLARE(tag) namespace vhs::traces { extern vhs::Trace trace_##tag; }
#define VHS_TRACE_DEFINE(tag) namespace vhs::traces { vhs::Trace trace_##tag { #tag, "VHS_TRACE_"#tag }; }
#define VHS_TRACE(tag, fmt, ...) vhs::traces::trace_##tag.print(FMT_STRING(fmt), ##__VA_ARGS__)


namespace vhs
{
    // Environment variable controlled tracing.
    class Trace
    {
    public:
        Trace() = delete;
        Trace(const Trace&) = delete;
        Trace(Trace&&) = delete;

        Trace(const char* tag, const char* env);
        ~Trace();


        Trace& operator=(const Trace&) = delete;
        Trace& operator=(Trace&&) = delete;


        template <class Fmt, class ... Args>
        void print(Fmt&& fmt, Args&&... args)
        {
            if (enabled_)
            {
                const auto now = std::chrono::high_resolution_clock::now();
                const auto now_ms = std::chrono::duration<double, std::milli> { now - start_time_ }.count();
                const auto thread = "main";

                const auto message = fmt::format(FMT_STRING("[{} {} {}] "), tag_, now_ms, thread)
                    + fmt::format(std::forward<Fmt>(fmt), std::forward<Args>(args)...) + "\n";

                std::cout << message;
            }
        }

    private:
        static std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
        static bool start_time_initialised_;

        const char* tag_;
        bool enabled_ = false;
    };
}

#endif
