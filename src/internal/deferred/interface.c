/**
 * \file
 * \brief Deferred logging interface implementation
 * \author Vladimir Petrigo
 */

#include "internal/interface.h"
#include "internal/config.h"
#include "internal/utils.h"
#include "list.h"

#include <lwrb/lwrb.h>
#include <printf/printf.h>

struct logger_ctx {
    lwrb_t ring_buf;
    enum mulog_log_level global_level;
};

struct out_function {
    mulog_log_output_fn output;
    enum mulog_log_level log_level;
    struct list_node node;
};

#ifndef MULOG_OUTPUT_HANDLERS
#error "Define MULOG_OUTPUT_HANDLERS to a number of maximum output handlers that can be registered"
#endif

struct handles {
    LIST_HEAD_VAR(out_functions);
    size_t out_count;
    struct out_function fns[MULOG_OUTPUT_HANDLERS];
};

static struct logger_ctx log_ctx = {
    .global_level = MULOG_LOG_LVL_DEBUG,
};
static struct handles handles = {
    .out_functions = LIST_HEAD_INIT_VAR,
};

/**
 * \brief Outputs a log entry to all registered output functions.
 *
 * This function iterates through all registered output functions and writes the
 * provided log entry buffer to each of them.
 *
 * \param buf Pointer to the buffer containing the log entry to be output.
 * \param buf_size Size of the buffer in bytes.
 */
static void output_log_entry(const char *buf, const size_t buf_size)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        fn->output(buf, buf_size);
    }
}

/**
 * \brief Prepend a timestamp to the specified ring buffer.
 *
 * This function generates a timestamp string in the format "sssssss.mmm " where
 * sssssss represents the seconds and mmm represents the milliseconds, and writes
 * it to the specified ring buffer if timestamp logging is enabled.
 *
 * \param ring_buf Pointer to the ring buffer where the timestamp will be written.
 * \return The number of bytes written to the ring buffer. Returns 0 if timestamp
 * logging is disabled.
 */
static inline size_t prepend_timestamp_rb(lwrb_t *ring_buf)
{
#if defined(MULOG_ENABLE_TIMESTAMP) && MULOG_ENABLE_TIMESTAMP == 1
    char timestamp_buffer[16];
    const unsigned long timestamp_ms = mulog_config_mulog_timestamp_get();
    const unsigned long ms = timestamp_ms % 1000;
    const unsigned long sec = timestamp_ms / 1000;
    const int ret =
        snprintf_(timestamp_buffer, ARRAY_SIZE(timestamp_buffer), "%07lu.%03lu ", sec, ms);

    if (ret < 0) {
        return 0;
    }

    return lwrb_write(ring_buf, timestamp_buffer, ret);
#else
    UNUSED(buf);
    UNUSED(buf_size);
    return 0;
#endif /* MULOG_ENABLE_TIMESTAMP */
}

static inline size_t prepend_level_rb(enum mulog_log_level level, lwrb_t *ring_buf)
{
    const char *level_str[] = {
        [MULOG_LOG_LVL_TRACE] = MULOG_TRACE_LVL, [MULOG_LOG_LVL_DEBUG] = MULOG_DEBUG_LVL,
        [MULOG_LOG_LVL_INFO] = MULOG_INFO_LVL,   [MULOG_LOG_LVL_WARNING] = MULOG_WARNING_LVL,
        [MULOG_LOG_LVL_ERROR] = MULOG_ERROR_LVL,
    };
    const char *trailing_symbols = ": ";
    // we need to write log level + ':' + ' '
    const size_t trailing_size = 2;
    const size_t level_size = strlen(level_str[level]);
    const size_t to_output = level_size + trailing_size;
    const size_t size = lwrb_get_free(ring_buf);

    if (size < to_output) {
        return 0;
    }

    size_t written = 0;

    written += lwrb_write(ring_buf, level_str[level], level_size);
    written += lwrb_write(ring_buf, trailing_symbols, trailing_size);

    return written;
}

enum mulog_ret_code interface_add_output_default(const mulog_log_output_fn output)
{
    return interface_add_output(output, log_ctx.global_level);
}

enum mulog_ret_code interface_add_output(const mulog_log_output_fn output,
                                         const enum mulog_log_level log_level)
{
    if (output == NULL || log_level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    if (log_ctx.global_level != log_level) {
        return MULOG_RET_CODE_UNSUPPORTED;
    }

    struct out_function *fn = NULL;

    for (size_t i = 0; i < ARRAY_SIZE(handles.fns); ++i) {
        if (LIST_NODE_IS_DANGLING(&handles.fns[i].node)) {
            fn = &handles.fns[i];
            break;
        }
    }

    if (fn == NULL) {
        return MULOG_RET_CODE_NO_MEM;
    }

    LIST_NODE_INIT(&fn->node);
    ++handles.out_count;
    fn->output = output;
    fn->log_level = log_level;
    list_head_add(&handles.out_functions, &fn->node);

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code interface_set_log_buffer(char *log_buffer, const size_t log_buffer_size)
{
    return lwrb_init(&log_ctx.ring_buf, log_buffer, log_buffer_size) != 0
               ? MULOG_RET_CODE_OK
               : MULOG_RET_CODE_INVALID_ARG;
}

enum mulog_ret_code interface_set_global_log_level(const enum mulog_log_level log_level)
{
    if (log_level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    log_ctx.global_level = log_level;

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code interface_set_log_level_per_output(const enum mulog_log_level log_level,
                                                       const mulog_log_output_fn output)
{
    UNUSED(log_level);
    UNUSED(output);
    return MULOG_RET_CODE_UNSUPPORTED;
}

enum mulog_ret_code interface_unregister_output(const mulog_log_output_fn output)
{
    struct list_node *it = NULL;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *out = LIST_ENTRY(it, struct out_function, node);

        if (out->output == output) {
            list_head_del(it);
            --handles.out_count;
            break;
        }
    }

    if (it == NULL) {
        return MULOG_RET_CODE_NOT_FOUND;
    }

    return MULOG_RET_CODE_OK;
}

void interface_unregister_all_outputs(void)
{
    struct list_node *it = NULL;
    struct list_node *temp = NULL;

    LIST_FOR_EACH_SAFE(it, temp, &handles.out_functions)
    {
        list_head_del(it);
    }
}

void interface_reset(void)
{
    lwrb_reset(&log_ctx.ring_buf);
    lwrb_free(&log_ctx.ring_buf);
}

int interface_log_output(const enum mulog_log_level level, const char *fmt, va_list args)
{
    if (list_head_empty(&handles.out_functions) || lwrb_is_ready(&log_ctx.ring_buf) == 0 ||
        level >= MULOG_LOG_LVL_COUNT || level < log_ctx.global_level) {
        return 0;
    }

    size_t written = 0;
    size_t processed = prepend_timestamp_rb(&log_ctx.ring_buf);

    if (processed == 0) {
        return 0;
    }

    written += processed;
    processed = prepend_level_rb(level, &log_ctx.ring_buf);

    if (processed == 0) {
        return 0;
    }

    written += processed;
    va_list args_copy;
    va_copy(args_copy, args);
    const int ret = vsnprintf_(NULL, 0, fmt, args_copy);

    if (ret < 0) {
        return ret;
    }
    // TODO: Extract this to a configuration
    const size_t max_single_log_size = 128;
    const size_t available_size = lwrb_get_free(&log_ctx.ring_buf);
    size_t to_write = ret;

    if (to_write > max_single_log_size || to_write > available_size) {
        to_write = to_write > available_size ? available_size : max_single_log_size;
    }

    char buffer[to_write + 1];
    vsnprintf_(buffer, ARRAY_SIZE(buffer), fmt, args);

    written += lwrb_write(&log_ctx.ring_buf, buffer, to_write);
    written += lwrb_write(&log_ctx.ring_buf, MULOG_LOG_LINE_TERMINATION,
                          strlen(MULOG_LOG_LINE_TERMINATION));

    return written;
}

int interface_deferred_log(void)
{
    size_t data_size = lwrb_get_full(&log_ctx.ring_buf);
    const int ret = data_size;

    while (data_size > 0) {
        const char *read_buf = lwrb_get_linear_block_read_address(&log_ctx.ring_buf);
        size_t read_buf_size = lwrb_get_linear_block_read_length(&log_ctx.ring_buf);

        if (read_buf_size > data_size) {
            read_buf_size = data_size;
        }

        output_log_entry(read_buf, read_buf_size);
        lwrb_skip(&log_ctx.ring_buf, read_buf_size);
        data_size -= read_buf_size;
    }

    return ret;
}
