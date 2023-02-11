#include "helper.h"

uint64_t get_current_time()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t get_time_taken(uint64_t start_time)
{
    uint64_t end_time = get_current_time();
    return end_time - start_time;
}