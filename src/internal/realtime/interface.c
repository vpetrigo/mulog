/**
 * \file
 * \brief Realtime logging interface implementation
 * \author Vladimir Petrigo
 */

#include "internal/interface.h"
#include "internal/config.h"
#include "internal/utils.h"
#include "list.h"

#include <printf/printf.h>

// PRIVATE TYPE DECLARATIONS

/**
 * \brief Logger module context
 */
struct logger_ctx {
    enum mulog_log_level global_level; /**< Global log level used for init new outputs */
    char *log_buffer;                  /**< Log entry format buffer */
    size_t log_buffer_size;            /**< Size of the log entry buffer */
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

// PRIVATE VARIABLE DEFINITIONS

static struct logger_ctx log_ctx = {
    .global_level = MULOG_LOG_LVL_DEBUG,
};
static struct handles handles = {
    .out_functions = LIST_HEAD_INIT_VAR,
};

// PRIVATE FUNCTION DEFINITIONS

/**
 * \brief Sets the log level for all output functions.
 *
 * \param log_level The log level to set for all output functions.
 */
static void set_log_level_for_all_outputs(const enum mulog_log_level log_level)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        fn->log_level = log_level;
    }
}

/**
 * \brief Count the number of output functions with log level above a specified level.
 *
 * This function iterates through a list of output functions and counts how many
 * have a log level greater than or equal to the specified log level.
 *
 * \param log_level The log level to compare against.
 * \return The number of output functions with a log level above the specified level.
 */
static size_t get_num_outputs_above_level(const enum mulog_log_level log_level)
{
    struct list_node *it;
    size_t logger_count = 0;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *out = LIST_ENTRY(it, struct out_function, node);

        if (out->log_level <= log_level) {
            ++logger_count;
        }
    }

    return logger_count;
}

/**
 * \brief Outputs a log entry to all the configured output functions that have a log level
 *        lower than or equal to the specified log level.
 *
 * \param log_level The log level of the entry to be output.
 * \param buf The buffer containing the log entry.
 * \param buf_size The size of the buffer containing the log entry.
 */
static void output_log_entry(const enum mulog_log_level log_level, const char *buf,
                             const size_t buf_size)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        if (fn->log_level <= log_level) {
            fn->output(buf, buf_size);
        }
    }
}

/**
 * \brief Prepends a timestamp to the provided buffer.
 *
 * This function inserts a formatted timestamp at the beginning of the provided buffer,
 * if timestamping is enabled through the MULOG configuration. The timestamp format is
 * `sssssss.mmm ` where sssssss represents the seconds and mmm represents the milliseconds
 *
 * \param buf The buffer to which the timestamp will be prepended.
 * \param buf_size The size of the buffer.
 * \return The number of characters written to the buffer, or 0 if timestamping is disabled.
 */
static inline int prepend_timestamp(char *buf, const size_t buf_size)
{
#if defined(MULOG_ENABLE_TIMESTAMP) && MULOG_ENABLE_TIMESTAMP == 1
    const unsigned long timestamp_ms = mulog_config_mulog_timestamp_get();
    const unsigned long ms = timestamp_ms % 1000;
    const unsigned long sec = timestamp_ms / 1000;

    return snprintf_(buf, buf_size, "%07lu.%03lu ", sec, ms);
#else
    UNUSED(buf);
    UNUSED(buf_size);
    return 0;
#endif /* MULOG_ENABLE_TIMESTAMP */
}

/**
 * \brief Prepends the log level string to a buffer.
 *
 * This function takes a buffer and a log level, and prepends the corresponding log level string
 * representation to the buffer.
 *
 * \param buf A pointer to the buffer where the log level string will be prepended.
 * \param buf_size The size of the buffer.
 * \param level The log level to prepend.
 * \return The number of characters written to the buffer, or a negative value if an error occurs.
 */
static inline int prepend_level(char *buf, const size_t buf_size, const enum mulog_log_level level)
{
    const char *level_str[] = {
        [MULOG_LOG_LVL_TRACE] = MULOG_TRACE_LVL, [MULOG_LOG_LVL_DEBUG] = MULOG_DEBUG_LVL,
        [MULOG_LOG_LVL_INFO] = MULOG_INFO_LVL,   [MULOG_LOG_LVL_WARNING] = MULOG_WARNING_LVL,
        [MULOG_LOG_LVL_ERROR] = MULOG_ERROR_LVL,
    };

    return snprintf_(buf, buf_size, "%s: ", level_str[level]);
}

/**
 * \brief Appends a newline character to the given buffer.
 *
 * This function appends a newline character to the end of the buffer, ensuring
 * the buffer size constraint is respected.
 *
 * \param buf Buffer to which the newline character will be appended.
 * \param buf_size Size of the buffer.
 * \return The number of characters written (including the newline character)
 *         or a negative value if an error occurs.
 */
static inline int line_termination(char *buf, const size_t buf_size)
{
    return snprintf_(buf, buf_size, "%s", "\n");
}

// PUBLIC FUNCTION DEFINITIONS

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
    log_ctx.log_buffer = log_buffer;
    log_ctx.log_buffer_size = log_buffer_size;

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code interface_set_global_log_level(const enum mulog_log_level log_level)
{
    if (log_level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    log_ctx.global_level = log_level;
    set_log_level_for_all_outputs(log_ctx.global_level);

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code interface_set_log_level_per_output(const enum mulog_log_level log_level,
                                                       const mulog_log_output_fn output)
{
    if (log_level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        if (fn->output == output) {
            fn->log_level = log_level;

            return MULOG_RET_CODE_OK;
        }
    }

    return MULOG_RET_CODE_NOT_FOUND;
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
    handles.out_count = 0;
    log_ctx.log_buffer = NULL;
    log_ctx.log_buffer_size = 0;
    log_ctx.global_level = MULOG_LOG_LVL_DEBUG;
    LIST_HEAD_INIT(&handles.out_functions);

    for (size_t i = 0; i < ARRAY_SIZE(handles.fns); ++i) {
        LIST_NODE_INIT(&handles.fns[i].node);
    }
}

int interface_log_output(const enum mulog_log_level level, const char *fmt, va_list args)
{
    if (list_head_empty(&handles.out_functions) || log_ctx.log_buffer == NULL ||
        log_ctx.log_buffer_size == 0 || level >= MULOG_LOG_LVL_COUNT) {
        return 0;
    }

    const size_t logger_count = get_num_outputs_above_level(level);

    if (logger_count == 0) {
        return 0;
    }

    size_t offset = 0;
    int ret = 0;

    ret = prepend_timestamp(log_ctx.log_buffer, log_ctx.log_buffer_size);

    if (ret < 0) {
        return ret;
    }

    offset += ret;

    if (log_ctx.log_buffer_size < offset) {
        offset = (int)log_ctx.log_buffer_size - 1;
        goto exit;
    }

    ret = prepend_level(log_ctx.log_buffer + offset, log_ctx.log_buffer_size - offset, level);

    if (ret < 0) {
        return ret;
    }

    offset += ret;

    if (log_ctx.log_buffer_size < offset) {
        offset = log_ctx.log_buffer_size - 1;
        goto exit;
    }

    ret = vsnprintf_(log_ctx.log_buffer + offset, log_ctx.log_buffer_size - offset, fmt, args);

    if (ret < 0) {
        return ret;
    }

    offset += ret;

    if (log_ctx.log_buffer_size < offset) {
        offset = log_ctx.log_buffer_size - 1;
        goto exit;
    }

    ret = line_termination(log_ctx.log_buffer + offset, log_ctx.log_buffer_size - offset);

    if (ret < 0) {
        return ret;
    }

    offset += ret;

    if (log_ctx.log_buffer_size < offset) {
        offset = log_ctx.log_buffer_size - 1;
    }
exit:
    output_log_entry(level, log_ctx.log_buffer, offset);

    return (int)offset;
}

int interface_deferred_log(void)
{
    return MULOG_RET_CODE_UNSUPPORTED;
}
