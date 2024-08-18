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

static enum mulog_log_level mulog_log_level = MULOG_LOG_LVL_DEBUG;

struct out_function {
    mulog_log_output_fn output;
    struct list_node node;
};

struct handles {
    LIST_HEAD_VAR(out_functions);
    struct out_function fns[MULOG_OUTPUT_HANDLERS];
};

static struct handles handles = {
    .out_functions = LIST_HEAD_INIT_VAR,
};

static int prepend_level(char *buf, const enum mulog_log_level level)
{
    const char *level_str[] = {
        [MULOG_LOG_LVL_TRACE] = MULOG_TRACE_LVL, [MULOG_LOG_LVL_DEBUG] = MULOG_DEBUG_LVL,
        [MULOG_LOG_LVL_INFO] = MULOG_INFO_LVL,   [MULOG_LOG_LVL_WARNING] = MULOG_WARNING_LVL,
        [MULOG_LOG_LVL_ERROR] = MULOG_ERROR_LVL,
    };

    return sprintf_(buf, "%s: ", level_str[level]);
}

void mulog_set_log_level(const enum mulog_log_level level)
{
    mulog_log_level = level;
}

void mulog_add_output(const mulog_log_output_fn output)
{
    struct out_function *fn = NULL;

    for (size_t i = 0; i < ARRAY_SIZE(handles.fns); i++) {
        if (LIST_NODE_IS_DANGLING(&handles.fns[i].node)) {
            fn = &handles.fns[i];
            break;
        }
    }

    if (fn == NULL) {
        return;
    }

    LIST_NODE_INIT(&fn->node);
    fn->output = output;
    list_head_add(&handles.out_functions, &fn->node);
}

void mulog_remove_output(const mulog_log_output_fn output)
{
    struct list_node *it = NULL;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *out = LIST_ENTRY(it, struct out_function, node);

        if (out->output == output) {
            list_head_del(it);
            break;
        }
    }
}

MULOG_PRINTF_ATTR int mulog_log(const enum mulog_log_level level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    if (level < mulog_log_level || list_head_empty(&handles.out_functions)) {
        va_end(args);
        return 0;
    }

    static char buf[MULOG_SINGLE_LINE_LEN];

    const int offset = prepend_level(buf, level);
    const int ret = vsnprintf_(buf + offset, sizeof(buf) - offset, fmt, args);
    va_end(args);

    struct list_node *it;

    LIST_FOR_EACH(it, &handles.out_functions)
    {
        const struct out_function *fn = LIST_ENTRY(it, struct out_function, node);

        fn->output(buf);
    }

    return ret;
}
