/**
 * \file
 * \brief Example with mulog in realtime output mode
 * \author Vladimir Petrigo
 */
#include "mulog.h"

#include <stdbool.h>
#include <stdio.h>

#include <sys/time.h>

static char buffer[256];

unsigned long mulog_config_mulog_timestamp_get(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool mulog_config_mulog_lock(void)
{
    return true;
}

void mulog_config_mulog_unlock(void)
{
}

static void output1_fn(const char *data, const size_t data_size)
{
    printf("Output 1: %.*s", (int)data_size, data);
}

static void output2_fn(const char *data, const size_t data_size)
{
    printf("Output 2: %.*s", (int)data_size, data);
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
