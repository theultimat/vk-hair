#include <cstdlib>

#include "trace.hpp"


namespace vhs
{
    Trace::Trace(const char* tag, const char* env) :
        tag_ { tag }
    {
        const auto value = std::getenv(env);
        enabled_ = value && *value != '0';

        if (!start_time_initialised_)
        {
            start_time_ = std::chrono::high_resolution_clock::now();
            start_time_initialised_ = true;
        }
    }

    Trace::~Trace()
    {
        std::cout << std::flush;
    }


    std::chrono::time_point<std::chrono::high_resolution_clock> Trace::start_time_;
    bool Trace::start_time_initialised_ = false;
}
