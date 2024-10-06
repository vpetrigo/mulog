/**
 * \file
 * \brief Example with mulog in realtime output mode
 * \author Vladimir Petrigo
 */
#include "mulog.h"

#include <stdio.h>
#include <time.h>

static char buffer[256];

unsigned long mulog_config_mulog_timestamp_get(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void output1_fn(const char *data, size_t data_size)
{
    printf("Output 1: %.*s", data_size, data);
}

static void output2_fn(const char *data, size_t data_size)
{
    printf("Output 2: %.*s", data_size, data);
}
// required here to facilitate libprintf dependency requirements
void putchar_(char c)
{
    (void)c;
}

int main(void)
{
    mulog_set_log_buffer(buffer, sizeof(buffer));
    mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    mulog_add_output(output1_fn);
    mulog_add_output_with_log_level(output2_fn, MULOG_LOG_LVL_ERROR);

    MULOG_LOG_DBG("Hello");
    MULOG_LOG_TRACE("World!");
    MULOG_LOG_ERR("Error: 123456");

    return 0;
}
