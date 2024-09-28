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

static void set_log_level_for_all_outputs(const enum mulog_log_level log_level)
{
    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        fn->log_level = log_level;
    }
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

static inline int prepend_level(char *buf, const size_t buf_size, const enum mulog_log_level level)
{
    const char *level_str[] = {
        [MULOG_LOG_LVL_TRACE] = MULOG_TRACE_LVL, [MULOG_LOG_LVL_DEBUG] = MULOG_DEBUG_LVL,
        [MULOG_LOG_LVL_INFO] = MULOG_INFO_LVL,   [MULOG_LOG_LVL_WARNING] = MULOG_WARNING_LVL,
        [MULOG_LOG_LVL_ERROR] = MULOG_ERROR_LVL,
    };

    return snprintf_(buf, buf_size, "%s: ", level_str[level]);
}

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
}

int interface_log_output(const enum mulog_log_level level, const char *fmt, const va_list args)
{
    if (list_head_empty(&handles.out_functions) || log_ctx.log_buffer == NULL ||
        log_ctx.log_buffer_size == 0 || level >= MULOG_LOG_LVL_COUNT) {
        return 0;
    }

    const size_t logger_count = get_num_outputs_above_level(level);

    if (logger_count == 0) {
        return 0;
    }

    int offset = prepend_level(log_ctx.log_buffer, log_ctx.log_buffer_size, level);
    const int ret =
        vsnprintf_(log_ctx.log_buffer + offset, log_ctx.log_buffer_size - offset, fmt, args);

    if (ret < 0) {
        return ret;
    }

    offset += ret;
    output_log_entry(level, log_ctx.log_buffer, (size_t)offset);

    return ret;
}

int interface_deferred_log(void)
{
    return MULOG_RET_CODE_UNSUPPORTED;
}
