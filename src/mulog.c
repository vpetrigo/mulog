/**
 * \file
 * \brief
 * \author Vladimir Petrigo
 */

#include "mulog.h"
#include "internal/interface.h"

#include <internal/utils.h>
#include <stdarg.h>

// PUBLIC FUNCTION DEFINITIONS

enum mulog_ret_code mulog_set_log_buffer(char *buf, const size_t buf_size)
{
    return interface_set_log_buffer(buf, buf_size);
}

enum mulog_ret_code mulog_set_log_level(const enum mulog_log_level level)
{
    return interface_set_global_log_level(level);
}

enum mulog_ret_code mulog_set_channel_log_level(const mulog_log_output_fn output,
                                                const enum mulog_log_level level)
{
    return interface_set_log_level_per_output(level, output);
}

enum mulog_ret_code mulog_add_output(const mulog_log_output_fn output)
{
    return interface_add_output_default(output);
}

enum mulog_ret_code mulog_add_output_with_log_level(const mulog_log_output_fn output,
                                                    const enum mulog_log_level level)
{
    return interface_add_output(output, level);
}

enum mulog_ret_code mulog_unregister_output(const mulog_log_output_fn output)
{
    return interface_unregister_output(output);
}

void mulog_unregister_all_outputs(void)
{
    interface_unregister_all_outputs();
}

void mulog_reset(void)
{
    mulog_unregister_all_outputs();
    interface_reset();
}

int mulog_deferred_process(void)
{
    return interface_deferred_log();
}

MULOG_PRINTF_ATTR int mulog_log(const enum mulog_log_level level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    const int ret = interface_log_output(level, fmt, args);
    va_end(args);

    return ret;
}
