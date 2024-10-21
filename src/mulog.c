/**
 * \file
 * \brief
 * \author Vladimir Petrigo
 */

#include "mulog.h"
#include "internal/config.h"
#include "internal/interface.h"

#include <internal/utils.h>
#include <stdarg.h>

// PUBLIC FUNCTION DEFINITIONS

enum mulog_ret_code mulog_set_log_buffer(char *buf, const size_t buf_size)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_set_log_buffer(buf, buf_size);
    mulog_config_mulog_unlock();

    return ret;
}

enum mulog_ret_code mulog_set_log_level(const enum mulog_log_level level)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_set_global_log_level(level);
    mulog_config_mulog_unlock();

    return ret;
}

enum mulog_ret_code mulog_set_channel_log_level(const mulog_log_output_fn output,
                                                const enum mulog_log_level level)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_set_log_level_per_output(level, output);
    mulog_config_mulog_unlock();

    return ret;
}

enum mulog_ret_code mulog_add_output(const mulog_log_output_fn output)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_add_output_default(output);
    mulog_config_mulog_unlock();

    return ret;
}

enum mulog_ret_code mulog_add_output_with_log_level(const mulog_log_output_fn output,
                                                    const enum mulog_log_level level)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_add_output(output, level);
    mulog_config_mulog_unlock();

    return ret;
}

enum mulog_ret_code mulog_unregister_output(const mulog_log_output_fn output)
{
    if (!mulog_config_mulog_lock()) {
        return MULOG_RET_CODE_LOCK_FAILED;
    }

    const int ret = interface_unregister_output(output);
    mulog_config_mulog_unlock();

    return ret;
}

void mulog_unregister_all_outputs(void)
{
    if (!mulog_config_mulog_lock()) {
        return;
    }

    interface_unregister_all_outputs();
    mulog_config_mulog_unlock();
}

void mulog_reset(void)
{
    if (!mulog_config_mulog_lock()) {
        return;
    }

    interface_unregister_all_outputs();
    interface_reset();
    mulog_config_mulog_unlock();
}

int mulog_deferred_process(void)
{
    if (!mulog_config_mulog_lock()) {
        return 0;
    }

    const int ret = interface_deferred_log();
    mulog_config_mulog_unlock();

    return ret;
}

MULOG_PRINTF_ATTR int mulog_log(const enum mulog_log_level level, const char *fmt, ...)
{
    va_list args;

    if (!mulog_config_mulog_lock()) {
        return 0;
    }

    va_start(args, fmt);
    const int ret = interface_log_output(level, fmt, args);
    va_end(args);
    mulog_config_mulog_unlock();

    return ret;
}
