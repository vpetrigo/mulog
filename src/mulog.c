/**
 * \file
 * \brief
 * \author Vladimir Petrigo
 */

#include "mulog.h"
#if defined(MULOG_ENABLE_COLOR) && MULOG_ENABLE_COLOR == 1
#include "color.h"
#endif
#include "internal/config.h"
#include "list.h"

#include <printf/printf.h>

#include <stdarg.h>
#include <stdio.h>

// PRIVATE MACRO DEFINITIONS

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifndef MULOG_OUTPUT_HANDLERS
#error "Define MULOG_OUTPUT_HANDLERS to a number of maximum output handlers that can be registered"
#endif

#define MULOG_TRACE   "[TRACE]"
#define MULOG_DEBUG   "[DEBUG]"
#define MULOG_INFO    "[INFO]"
#define MULOG_WARNING "[WARN]"
#define MULOG_ERROR   "[ERROR]"

#if defined(MULOG_ENABLE_COLOR) && MULOG_ENABLE_COLOR == 1
#define MULOG_COLOR_TRACE   MULOG_COLOR_MAG MULOG_TRACE MULOG_COLOR_RESET
#define MULOG_COLOR_DEBUG   MULOG_COLOR_BLU MULOG_DEBUG MULOG_COLOR_RESET
#define MULOG_COLOR_INFO    MULOG_COLOR_GRN MULOG_INFO MULOG_COLOR_RESET
#define MULOG_COLOR_WARNING MULOG_COLOR_YEL MULOG_WARNING MULOG_COLOR_RESET
#define MULOG_COLOR_ERROR   MULOG_COLOR_RED MULOG_ERROR MULOG_COLOR_RESET

#define MULOG_TRACE_LVL   MULOG_COLOR_TRACE
#define MULOG_DEBUG_LVL   MULOG_COLOR_DEBUG
#define MULOG_INFO_LVL    MULOG_COLOR_INFO
#define MULOG_WARNING_LVL MULOG_COLOR_WARNING
#define MULOG_ERROR_LVL   MULOG_COLOR_ERROR
#else
#define MULOG_TRACE_LVL   MULOG_TRACE
#define MULOG_DEBUG_LVL   MULOG_DEBUG
#define MULOG_INFO_LVL    MULOG_INFO
#define MULOG_WARNING_LVL MULOG_WARNING
#define MULOG_ERROR_LVL   MULOG_ERROR
#endif

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

struct handles {
    LIST_HEAD_VAR(out_functions);
    size_t out_count;
    struct out_function fns[MULOG_OUTPUT_HANDLERS];
};

// PRIVATE VARIABLE DEFINITIONS

static struct logger_ctx logger_ctx = {
    .global_level = MULOG_LOG_LVL_DEBUG,
    .log_buffer = NULL,
    .log_buffer_size = 0,
};

static struct handles handles = {
    .out_functions = LIST_HEAD_INIT_VAR,
};

// PRIVATE FUNCTION DEFINITIONS

static int prepend_level(char *buf, const enum mulog_log_level level)
{
    const char *level_str[] = {
        [MULOG_LOG_LVL_TRACE] = MULOG_TRACE_LVL, [MULOG_LOG_LVL_DEBUG] = MULOG_DEBUG_LVL,
        [MULOG_LOG_LVL_INFO] = MULOG_INFO_LVL,   [MULOG_LOG_LVL_WARNING] = MULOG_WARNING_LVL,
        [MULOG_LOG_LVL_ERROR] = MULOG_ERROR_LVL,
    };

    return sprintf_(buf, "%s: ", level_str[level]);
}

static enum mulog_ret_code add_output(const mulog_log_output_fn output,
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

static void output_log_entry(const enum mulog_log_level log_level, const char *buf)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        if (fn->log_level <= log_level) {
            fn->output(buf);
        }
    }
}

static void set_log_level_for_all_outputs(const enum mulog_log_level log_level)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        fn->log_level = log_level;
    }
}

static enum mulog_ret_code set_log_level_for_output(mulog_log_output_fn output,
                                                    enum mulog_log_level level)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        if (fn->output == output) {
            fn->log_level = level;

            return MULOG_RET_CODE_OK;
        }
    }

    return MULOG_RET_CODE_NOT_FOUND;
}

// PUBLIC FUNCTION DEFINITIONS

enum mulog_ret_code mulog_set_log_buffer(char *buf, const size_t buf_size)
{
    logger_ctx.log_buffer = buf;
    logger_ctx.log_buffer_size = buf_size;

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code mulog_set_log_level(const enum mulog_log_level level)
{
    if (level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    logger_ctx.global_level = level;
    set_log_level_for_all_outputs(level);

    return MULOG_RET_CODE_OK;
}

enum mulog_ret_code mulog_set_channel_log_level(const mulog_log_output_fn output,
                                                const enum mulog_log_level level)
{
    if (level >= MULOG_LOG_LVL_COUNT) {
        return MULOG_RET_CODE_INVALID_ARG;
    }

    return set_log_level_for_output(output, level);
}

enum mulog_ret_code mulog_add_output(const mulog_log_output_fn output)
{
    return add_output(output, logger_ctx.global_level);
}

enum mulog_ret_code mulog_add_output_with_log_level(const mulog_log_output_fn output,
                                                    const enum mulog_log_level level)
{
    return add_output(output, level);
}

enum mulog_ret_code mulog_remove_output(const mulog_log_output_fn output)
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

    if (handles.out_count == 0) {
        LIST_HEAD_INIT(&handles.out_functions);
    }

    return MULOG_RET_CODE_OK;
}

void mulog_remove_all_outputs(void)
{
    struct list_node *it = NULL;
    struct list_node *temp = NULL;

    LIST_FOR_EACH_SAFE(it, temp, &handles.out_functions)
    {
        list_head_del(it);
    }
}

MULOG_PRINTF_ATTR int mulog_log(const enum mulog_log_level level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    if (list_head_empty(&handles.out_functions) || logger_ctx.log_buffer == NULL ||
        logger_ctx.log_buffer_size == 0) {
        va_end(args);
        return 0;
    }

    const size_t logger_count = get_num_outputs_above_level(level);

    if (logger_count == 0) {
        va_end(args);
        return 0;
    }

    const int offset = prepend_level(logger_ctx.log_buffer, level);
    const int ret =
        vsnprintf_(logger_ctx.log_buffer + offset, logger_ctx.log_buffer_size - offset, fmt, args);

    va_end(args);
    output_log_entry(level, logger_ctx.log_buffer);

    return ret;
}
