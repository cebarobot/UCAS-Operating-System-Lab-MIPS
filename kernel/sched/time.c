#include "time.h"

uint64_t time_elapsed = 0;
static uint32_t last_cp0_count;

static int MHZ = 300;

uint64_t get_ticks()
{
    return time_elapsed;
}

uint64_t get_timer()
{
    return time_elapsed / (300000);
    // return time_elapsed / (300000000);
}

void update_time_elapsed() {
    uint32_t current = get_cp0_count();

    if (current > last_cp0_count)
    {
        time_elapsed = time_elapsed + current - last_cp0_count;
    }
    else
    {
        time_elapsed = time_elapsed + 0xffffffff + current - last_cp0_count;
    }

    last_cp0_count = current;
}


void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();
    
    while (get_timer() - begin_time < time)
    {
    };
    return;
}